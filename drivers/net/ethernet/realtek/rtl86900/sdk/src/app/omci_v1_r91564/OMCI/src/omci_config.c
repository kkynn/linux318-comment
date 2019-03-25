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
 * Purpose : Definition of OMCI config APIs
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) OMCI config APIs
 */

#include "app_basic.h"
#include "omci_task.h"
#ifndef OMCI_X86
#include "rtk/svlan.h"
#include "common/error.h"
#include "rtk/switch.h"
#include "rtk/stat.h"
#include "rtk/gpon.h"
#include <hal/common/halctrl.h>
#endif
#include "feature_mgmt.h"

#define OMCI_GET_TPID_VALUE(tpid) (((tpid) == 0x8100 ? TPID_8100 : ((tpid) == 0x9100 ? TPID_9100 : TPID_88A8)))

#define DROP_UNTAG(op) ({                           \
    int __ret = FALSE;                          \
    if ((VTFD_FWD_OP_VID_F_UNTAG_C == op) ||    \
        (VTFD_FWD_OP_TCI_F_UNTAG_C == op) ||    \
        (VTFD_FWD_OP_PRI_F_UNTAG_C == op) ||    \
        (VTFD_FWD_OP_PRI_H_UNTAG_C == op) ||    \
        (VTFD_FWD_OP_VID_H_UNTAG_C == op) ||    \
        (VTFD_FWD_OP_TCI_H_UNTAG_C == op))      \
    {                                           \
        __ret = TRUE;                           \
    }                                           \
    __ret;                                      \
})


#define CARE_VID(op) ({                         \
    int __ret = FALSE;                          \
    if ((VTFD_FWD_OP_VID_F_UNTAG_C == op) ||    \
        (VTFD_FWD_OP_TCI_F_UNTAG_C == op) ||    \
        (VTFD_FWD_OP_VID_H_UNTAG_A == op) ||    \
        (VTFD_FWD_OP_TCI_H_UNTAG_A == op) ||    \
        (VTFD_FWD_OP_VID_H_UNTAG_C == op) ||    \
        (VTFD_FWD_OP_TCI_H_UNTAG_C == op))      \
    {                                           \
        __ret = TRUE;                           \
    }                                           \
    __ret;                                      \
})

#define CARE_PRI(op) ({                         \
    int __ret = FALSE;                          \
    if ((VTFD_FWD_OP_PRI_F_UNTAG_C == op) ||    \
        (VTFD_FWD_OP_PRI_H_UNTAG_A == op) ||    \
        (VTFD_FWD_OP_PRI_H_UNTAG_C == op))      \
    {                                           \
        __ret = TRUE;                           \
    }                                           \
    __ret;                                      \
})


extern int MIB_TABLE_LAST_INDEX;

typedef struct WORK_MSG_HDR_T{
    long msgType;
    MIB_TABLE_INDEX tableId;
    void *OldRow;
    void *NewRow;
    MIB_OPERA_TYPE operType;
    MIB_ATTRS_SET attrSet;
    UINT32 pri;
} WORK_MSG_HDR_T;



/**********PM access protection************/
static pthread_mutex_t  gOmciPmAccessEthCntMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t  gOmciPmAccessGemCntMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t  gOmciPmAccessRtpCntMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t  gOmciPmAccessCcpmCntMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t  gOmciPmAccessSipagpmCntMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t  gOmciPmAccessSipcipmCntMutex = PTHREAD_MUTEX_INITIALIZER;


#define OMCI_GET_PM_ETH_LOCK    (pthread_mutex_lock(&gOmciPmAccessEthCntMutex))
#define OMCI_GET_PM_ETH_UNLOCK  (pthread_mutex_unlock(&gOmciPmAccessEthCntMutex))

#define OMCI_GET_PM_GEM_LOCK    (pthread_mutex_lock(&gOmciPmAccessGemCntMutex))
#define OMCI_GET_PM_GEM_UNLOCK  (pthread_mutex_unlock(&gOmciPmAccessGemCntMutex))

#define OMCI_GET_PM_RTP_LOCK    (pthread_mutex_lock(&gOmciPmAccessRtpCntMutex))
#define OMCI_GET_PM_RTP_UNLOCK  (pthread_mutex_unlock(&gOmciPmAccessRtpCntMutex))

#define OMCI_GET_PM_CCPM_LOCK    (pthread_mutex_lock(&gOmciPmAccessCcpmCntMutex))
#define OMCI_GET_PM_CCPM_UNLOCK  (pthread_mutex_unlock(&gOmciPmAccessCcpmCntMutex))

#define OMCI_GET_PM_SIPAGPM_LOCK    (pthread_mutex_lock(&gOmciPmAccessSipagpmCntMutex))
#define OMCI_GET_PM_SIPAGPM_UNLOCK  (pthread_mutex_unlock(&gOmciPmAccessSipagpmCntMutex))


#define OMCI_GET_PM_SIPCIPM_LOCK    (pthread_mutex_lock(&gOmciPmAccessSipcipmCntMutex))
#define OMCI_GET_PM_SIPCIPM_UNLOCK  (pthread_mutex_unlock(&gOmciPmAccessSipcipmCntMutex))

static GOS_ERROR_CODE omci_McTransparentFwd(MIB_TREE_CONN_T *pConn, omci_vlan_rule_t *pTrafficRule);


static GOS_ERROR_CODE omci_meOperCfg(MIB_TABLE_INDEX tableId, void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
    GOS_ERROR_CODE  ret = GOS_OK;
    MIB_TABLE_T     *pTable = mib_GetTablePtr(tableId);
    MIB_ENTRY_T     *pEntry;
    BOOL            isDisabled = FALSE;

    if(!pTable || !pTable->meOper)
        return GOS_OK;

    if(pTable->meOper->meOperConnCfg && operType !=MIB_GET)
        ret = pTable->meOper->meOperConnCfg(pOldRow,pNewRow,operType, attrSet, pri);

    if(pTable->meOper->meOperDrvCfg)
    {
        LIST_FOREACH(pEntry, &pTable->entryHead, entries)
        {
            if (0 == mib_CompareEntry(tableId, pEntry->pData, pNewRow))
            {
                isDisabled = pEntry->isDisabled;
                break;
            }
        }

        if (!isDisabled)
            ret = pTable->meOper->meOperDrvCfg(pOldRow,pNewRow,operType, attrSet, pri);
    }

    return ret;

}


GOS_ERROR_CODE OMCI_MeOperCfg(MIB_TABLE_INDEX tableId, void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
    omci_meOperCfg(tableId,pOldRow,pNewRow,operType, attrSet, pri);
    return GOS_OK;
}

GOS_ERROR_CODE OMCI_MeOperDumpMib(MIB_TABLE_INDEX tableId,void *pData)
{
    GOS_ERROR_CODE ret = GOS_OK;

    MIB_TABLE_T *pTable = mib_GetTablePtr(tableId);

    if(pTable && pTable->meOper && pTable->meOper->meOperDump)
    {
        ret = pTable->meOper->meOperDump(pData, pTable->pTableInfo);
    }
    return ret;
}

GOS_ERROR_CODE OMCI_MeOperConnCheck(MIB_TABLE_INDEX tableId,MIB_TREE_T *pTree,MIB_TREE_CONN_T *pConn,
omci_me_instance_t entityId,int parm)
{
    GOS_ERROR_CODE ret = GOS_OK;
    MIB_TABLE_T *pTable = mib_GetTablePtr(tableId);
    if(pTable && pTable->meOper && pTable->meOper->meOperConnCheck)
    {
        ret = pTable->meOper->meOperConnCheck(pTree,pConn,entityId,parm);
    }
    return ret;
}

GOS_ERROR_CODE OMCI_MeOperAvlTreeAdd(MIB_TABLE_INDEX tableId,MIB_TREE_T* pTree,omci_me_instance_t entityId)
{
    GOS_ERROR_CODE ret = GOS_OK;

    MIB_TABLE_T *pTable = mib_GetTablePtr(tableId);

    if(pTable && pTable->meOper && pTable->meOper->meOperAvlTreeAdd)
    {
        ret = pTable->meOper->meOperAvlTreeAdd(pTree,entityId);
    }
    return ret;
}

GOS_ERROR_CODE OMCI_MeOperAvlTreeDel(MIB_TABLE_INDEX tableId,MIB_TREE_T* pTree,omci_me_instance_t entityId)
{
    GOS_ERROR_CODE ret = GOS_OK;

    MIB_TABLE_T *pTable = mib_GetTablePtr(tableId);

    if(pTable && pTable->meOper && pTable->meOper->meOperAvlTreeDel)
    {
        ret = pTable->meOper->meOperAvlTreeDel(pTree,entityId);
    }
    return ret;
}

/*
*
* CLI Handler
*
*/

static void omci_mib_show(MIB_TABLE_T *pTable, int instanceId)
{
    MIB_TABLE_INDEX     tableId;
    MIB_ENTRY_T         *pEntry;
    omci_me_instance_t  instId;

    if (!pTable)
        return;

    tableId = pTable->tableIndex;

    OMCI_PRINT("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
    OMCI_PRINT("%s", MIB_GetTableName(tableId));
    OMCI_PRINT("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");

    LIST_FOREACH(pEntry, &pTable->entryHead, entries)
    {
        memcpy(&instId, pEntry->pData, sizeof(omci_me_instance_t));

        if (instanceId < 0 || instId == instanceId)
        {
            OMCI_PRINT("=================================");
            OMCI_MeOperDumpMib(tableId, pEntry->pData);
            OMCI_PRINT("=================================");
        }
    }
}

GOS_ERROR_CODE OMCI_TreeDump_Cmd(int key)
{
    int oldstdout = -1, nul = -1;
    const int stdout_fd = fileno(stdout);
    nul = omci_open_cli_fd();

    if (-1 != nul)
    {
        oldstdout = dup(stdout_fd);
        dup2(nul, stdout_fd);
        close(nul);
    }

    if( -1 == key)
        MIB_AvlTreeDump();
    else
        MIB_AvlTreeDumpByKey(key);

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

GOS_ERROR_CODE
OMCI_LogSet_Cmd(
    int usrDbgLvl,
    int drvDbgLvl)
{
    gUsrLogLevel = usrDbgLvl;
    OMCI_PRINT("Set User log level %u", usrDbgLvl);
    gDrvLogLevel = drvDbgLvl;
    omci_wrapper_setLog(gDrvLogLevel);

    return GOS_OK;
}

GOS_ERROR_CODE OMCI_LogGet_Cmd()
{
    int oldstdout = -1, nul = -1;
    const int stdout_fd = fileno(stdout);
    nul = omci_open_cli_fd();

    if (-1 != nul)
    {
        oldstdout = dup(stdout_fd);
        dup2(nul, stdout_fd);
        close(nul);
    }

    switch(gUsrLogLevel)
    {
    case OMCI_LOG_LEVEL_OFF:
        OMCI_PRINT("Usr Log Lvl: OFF");
        break;
    case OMCI_LOG_LEVEL_ERR:
        OMCI_PRINT("Usr Log Lvl: Error");
        break;
    case OMCI_LOG_LEVEL_WARN:
        OMCI_PRINT("Usr Log Lvl: Warning");
        break;
    case OMCI_LOG_LEVEL_INFO:
        OMCI_PRINT("Usr Log Lvl: Info");
        break;
    case OMCI_LOG_LEVEL_DBG:
        OMCI_PRINT("Usr Log Lvl: Debug");
        break;
    default:
        OMCI_PRINT("Usr Log Lvl: Get log level failed");
        break;
    }

    switch(gDrvLogLevel)
    {
    case OMCI_LOG_LEVEL_OFF:
        OMCI_PRINT("Drv Log Lvl: OFF");
            break;
    case OMCI_LOG_LEVEL_ERR:
        OMCI_PRINT("Drv Log Lvl: Error");
            break;
    case OMCI_LOG_LEVEL_WARN:
        OMCI_PRINT("Drv Log Lvl: Warning");
            break;
    case OMCI_LOG_LEVEL_INFO:
        OMCI_PRINT("Drv Log Lvl: Info");
            break;
    case OMCI_LOG_LEVEL_DBG:
        OMCI_PRINT("Drv Log Lvl: Debug");
            break;
    default:
        OMCI_PRINT("Drv Log Lvl: Get log level failed");
        break;
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

GOS_ERROR_CODE OMCI_PriQtoTcontDump_Cmd(void)
{
    int oldstdout = -1, nul = -1;
    const int stdout_fd = fileno(stdout);
    nul = omci_open_cli_fd();

    if (-1 != nul)
    {
        oldstdout = dup(stdout_fd);
        dup2(nul, stdout_fd);
        close(nul);
    }

    omci_wrapper_dumpPQ2TC();
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

GOS_ERROR_CODE OMCI_ConnDump_Cmd(void)
{
    int oldstdout = -1, nul = -1;
    const int stdout_fd = fileno(stdout);
    nul = omci_open_cli_fd();

    if (-1 != nul)
    {
        oldstdout = dup(stdout_fd);
        dup2(nul, stdout_fd);
        close(nul);
    }

    MIB_TreeConnDump();

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

GOS_ERROR_CODE OMCI_MibDump_Cmd(int tableIdx, char *tableName, int instanceId)
{
    MIB_TABLE_T         *pTable;
    MIB_TABLE_INDEX     tableId;
    GOS_ERROR_CODE      ret = GOS_ERR_PARAM;
    int oldstdout = -1, nul = -1;
    const int stdout_fd = fileno(stdout);
    nul = omci_open_cli_fd();

    if (-1 != nul)
    {
        oldstdout = dup(stdout_fd);
        dup2(nul, stdout_fd);
        close(nul);
    }

    if (tableIdx < 0)
    {
        for (tableId = MIB_TABLE_FIRST_INDEX;
                tableId <= MIB_TABLE_LAST_INDEX; tableId = MIB_TABLE_NEXT_INDEX(tableId))
        {
            pTable = mib_GetTablePtr(tableId);
            if (!pTable)
                goto done;

            omci_mib_show(pTable, -1);
        }
    }
    else
    {
        if (GOS_OK == omci_util_is_digit(tableName))
            tableId = MIB_GetTableIndexByClassId(atoi(tableName));
        else
            tableId = MIB_GetTableIndexByName(tableName);
        if (MIB_TABLE_UNKNOWN_INDEX == tableId)
            goto done;

        pTable = mib_GetTablePtr(tableId);
        if (!pTable)
            goto done;

        omci_mib_show(pTable, instanceId);
    }
    ret = GOS_OK;
done:
    if (-1 != nul)
    {
        fflush(stdout);
        if (-1 != oldstdout)
        {
            dup2(oldstdout, stdout_fd);
            close(oldstdout);
        }
    }

    return ret;
}

static void omci_mib_pm_show(MIB_TABLE_T *pTable, int instanceId)
{
    MIB_TABLE_INDEX     tableId;
    MIB_ENTRY_T         *pEntry;
    omci_me_instance_t  instId;

    if (!pTable)
        return;

    tableId = pTable->tableIndex;

    OMCI_PRINT("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
    OMCI_PRINT("%s", MIB_GetTableName(tableId));
    OMCI_PRINT("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");

    LIST_FOREACH(pEntry, &pTable->entryHead, entries)
    {
        memcpy(&instId, pEntry->pData, sizeof(omci_me_instance_t));

        if (instanceId < 0 || instId == instanceId)
        {
            OMCI_PRINT("=================================");
            OMCI_MeOperDumpMib(tableId, pEntry->pPmCurrentBin);
            OMCI_PRINT("=================================");
        }
    }
}

GOS_ERROR_CODE OMCI_MibDump_PM_Cmd(char *tableName, int instanceId)
{
    MIB_TABLE_INDEX     tableId;
    MIB_TABLE_T         *pTable;

    if (GOS_OK == omci_util_is_digit(tableName))
        tableId = MIB_GetTableIndexByClassId(atoi(tableName));
    else
        tableId = MIB_GetTableIndexByName(tableName);
    if (MIB_TABLE_UNKNOWN_INDEX == tableId)
        return GOS_ERR_PARAM;

    pTable = mib_GetTablePtr(tableId);
    if (!pTable)
        return GOS_ERR_PARAM;

    if (OMCI_ME_TYPE_STANDARD_PM != pTable->pTableInfo->StdType &&
            OMCI_ME_TYPE_PROPRIETARY_PM != pTable->pTableInfo->StdType)
        return GOS_ERR_NOTSUPPORT;

    int oldstdout = -1, nul = -1;
    const int stdout_fd = fileno(stdout);
    nul = omci_open_cli_fd();

    if (-1 != nul)
    {
        oldstdout = dup(stdout_fd);
        dup2(nul, stdout_fd);
        close(nul);
    }

    omci_mib_pm_show(pTable, instanceId);

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

GOS_ERROR_CODE OMCI_MibAttrGet_Cmd(UINT32 srcMsgQkey, int cid, unsigned short eid, int aid)
{
    PON_OMCI_CMD_T  msg;
    MIB_ENTRY_T     *pEntry = NULL;
    void            *pAttrData = NULL;
    MIB_TABLE_INDEX tableIndex;
    MIB_ATTR_INDEX  attrIndex;
    omci_me_class_t classIndex;
    UINT32          attrSize;
    size_t          val_size;
    unsigned int    idx;
    int             sum_len = 0;

    classIndex = (omci_me_class_t)cid;
    attrIndex = (MIB_ATTR_INDEX)aid;

    msg.cmd = PON_OMCI_CMD_MIB_ATTR_GET_RSP;
    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));
    msg.tableId = cid;
    msg.entityId = eid;
    msg.state = aid;

    if (MIB_TABLE_UNKNOWN_INDEX == (tableIndex = MIB_GetTableIndexByClassId(classIndex)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,
            "%s()@%d  Not support class index =%u\n", __FUNCTION__, __LINE__, classIndex);

        snprintf(msg.value, 2, "%s", "");
        goto send;
    }

    if (!(pEntry = mib_FindEntryByInstanceID(tableIndex, eid)))
    {

        OMCI_LOG(OMCI_LOG_LEVEL_WARN,
            "%s()@%d  Not support entity index =%u\n", __FUNCTION__, __LINE__, eid);

        snprintf(msg.value, 2, "%s", "");
        goto send;
    }

    if (!(pAttrData = mib_GetAttrPtr(tableIndex, pEntry->pData, attrIndex)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,
            "%s()@%d  Not support attribute index =%u\n", __FUNCTION__, __LINE__, attrIndex);

        snprintf(msg.value, 2, "%s", "");
        goto send;
    }

    attrSize = MIB_GetAttrLen(tableIndex, attrIndex);

    val_size = ((attrSize > sizeof(msg.value)) ? sizeof(msg.value) : attrSize);

    switch (MIB_GetAttrOutStyle(tableIndex, attrIndex))
    {
        case MIB_ATTR_OUT_CHAR:
            snprintf(msg.value, val_size, "%s", (CHAR *)pAttrData);
            break;
        case MIB_ATTR_OUT_DEC:
            switch (val_size)
            {
                case 1:
                    snprintf(msg.value, sizeof(msg.value), "%u", *((UINT8 *)pAttrData));
                    break;
                case 2:
                    snprintf(msg.value, sizeof(msg.value), "%u", *((UINT16 *)pAttrData));
                    break;
                case 4:
                    snprintf(msg.value, sizeof(msg.value), "%u", *((UINT32 *)pAttrData));
                    break;
                case 8:
                    snprintf(msg.value, sizeof(msg.value), "%lu", *((unsigned long int *)pAttrData));
                    break;
                default:
                    if (val_size > 8)
                    {
                        snprintf(msg.value, sizeof(msg.value), "%lu", *((unsigned long int *)pAttrData));
                    }
                    break;
            }
            break;
        case MIB_ATTR_OUT_HEX:
            switch (MIB_GetAttrDataType(tableIndex, attrIndex))
            {
                case MIB_ATTR_TYPE_STR:
                    snprintf(msg.value, val_size, "%s", (char *)pAttrData);
                    break;
                default:
                {
                    CHAR tmp[64];

                    for (idx = 0; idx < val_size; idx++)
                    {
                        sum_len += sprintf(tmp + sum_len, "%02x", *((UINT8 *)pAttrData + idx));
                    }
                    snprintf(msg.value, sizeof(msg.value), "%s", tmp);
                    break;
                }
            }
            break;
        default:
            snprintf(msg.value, 2, "%s", "");
            break;
    }

send:
    OMCI_SendMsg(srcMsgQkey, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

GOS_ERROR_CODE OMCI_ServiceFlowDump_Cmd(void)
{
    int oldstdout = -1, nul = -1;
    const int stdout_fd = fileno(stdout);
    nul = omci_open_cli_fd();

    if (-1 != nul)
    {
        oldstdout = dup(stdout_fd);
        dup2(nul, stdout_fd);
        close(nul);
    }

    omci_wrapper_dumpSrvId();
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

GOS_ERROR_CODE OMCI_SnSet_Cmd(unsigned char *serial)
{

    MIB_TABLE_ONTG_T ontg;

    if(MIB_GetFirst(MIB_TABLE_ONTG_INDEX,&ontg,sizeof(MIB_TABLE_ONTG_T))!=GOS_OK)
    {
        return GOS_FAIL;
    }

    memcpy(ontg.SerialNum,serial,sizeof(ontg.SerialNum));

    if(MIB_Set(MIB_TABLE_ONTG_INDEX,&ontg,sizeof(MIB_TABLE_ONTG_T))!=GOS_OK)
    {
        return GOS_FAIL;
    }

    omci_wrapper_setSerialNum(serial);
    return GOS_OK;
}


GOS_ERROR_CODE OMCI_SnGet_Cmd(void)
{
    MIB_TABLE_ONTG_T ontg;
    char serial[16]="";

    int oldstdout = -1, nul = -1;
    const int stdout_fd = fileno(stdout);
    nul = omci_open_cli_fd();

    if(MIB_GetFirst(MIB_TABLE_ONTG_INDEX,&ontg,sizeof(MIB_TABLE_ONTG_T))!=GOS_OK)
    {
        if (nul != -1)
        {
            close ( nul );
        }
        return GOS_FAIL;
    }

    if (-1 !=  nul)
    {
        oldstdout = dup(stdout_fd);
        dup2(nul, stdout_fd);
        close(nul);
    }

    OMCI_PRINT("SerialNumber: %c%c%c%c%02hhX%02hhX%02hhX%02hhX",
    ontg.SerialNum[0],ontg.SerialNum[1],
    ontg.SerialNum[2],ontg.SerialNum[3],
    ontg.SerialNum[4],ontg.SerialNum[5],
    ontg.SerialNum[6],ontg.SerialNum[7]);

    if (-1 != nul)
    {
        fflush(stdout);
        if (-1 != oldstdout)
        {
            dup2(oldstdout, stdout_fd);
            close(oldstdout);
        }
    }

    omci_wrapper_getSerialNum(serial);

    return GOS_OK;
}


GOS_ERROR_CODE OMCI_MibReset_Cmd(void)
{
    omci_wrapper_resetMib();
    OMCI_ResetMib();

    return GOS_OK;
}


GOS_ERROR_CODE OMCI_DevModeGet_Cmd(void)
{
    int oldstdout = -1, nul = -1;

    const int stdout_fd = fileno(stdout);

    nul = omci_open_cli_fd();
    if (-1 != nul)
    {
        oldstdout = dup(stdout_fd);
        dup2(nul, stdout_fd);
        close(nul);
    }

    switch(gInfo.devMode){
    case OMCI_DEV_MODE_ROUTER:
        OMCI_PRINT("DevMode: router\n");
    break;
    case OMCI_DEV_MODE_BRIDGE:
        OMCI_PRINT("DevMode: bridge\n");
    break;
    case OMCI_DEV_MODE_HYBRID:
        OMCI_PRINT("DevMode: hybrid\n");
    break;
    default:
    break;
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


GOS_ERROR_CODE OMCI_DevModeSet_Cmd(char *devMode)
{
    if(!strcmp(devMode,"router"))
    {
        gInfo.devMode = OMCI_DEV_MODE_ROUTER;
    }else
    if(!strcmp(devMode,"bridge"))
    {
        gInfo.devMode = OMCI_DEV_MODE_BRIDGE;
    }else
    if(!strcmp(devMode,"hybrid"))
    {
        gInfo.devMode = OMCI_DEV_MODE_HYBRID;
    }
    omci_wrapper_setDevMode(gInfo.devMode);
    return GOS_OK;
}


GOS_ERROR_CODE OMCI_MibSet_Cmd(int classId, UINT16 entityId, char *fileName, unsigned char *val)
{
    MIB_TABLE_INDEX tableId;
    MIB_ATTR_INDEX  attrIndex;
    UINT32          attrLen, entrySize;
    MIB_ATTR_TYPE   attrType;
    void            *pMibRowBuff = NULL;
    MIB_ATTRS_SET   validMibSet = 0;
    UINT8           buf[MIB_TABLE_ENTRY_MAX_SIZE];
    CHAR            oldMibRow[MIB_TABLE_ENTRY_MAX_SIZE];

    memset(buf, 0x0, MIB_TABLE_ENTRY_MAX_SIZE);
    pMibRowBuff = (void *)buf;
    tableId     = MIB_GetTableIndexByClassId((omci_me_class_t)classId);
    entrySize   = MIB_GetTableEntrySize(tableId);
    attrIndex   = MIB_GetAttrIndexByName(tableId, fileName);
    MIB_SetAttrSet(&validMibSet, attrIndex);
    attrLen     = MIB_GetAttrLen(tableId, attrIndex);
    attrType    = MIB_GetAttrDataType(tableId, attrIndex);
    MIB_SetAttrToBuf(tableId, MIB_ATTR_FIRST_INDEX, &entityId, pMibRowBuff, sizeof(entityId));

    // Check if entity exists or not
    if (GOS_OK != MIB_Get(tableId, pMibRowBuff, entrySize))
    {
        OMCI_PRINT("SingleSet: entity not exist in MIB, class=%d, entity=0x%x", classId, entityId);
        return GOS_FAIL;
    }

    memcpy(oldMibRow, pMibRowBuff, entrySize);

    switch (attrType)
    {
        case MIB_ATTR_TYPE_UINT8:
        case MIB_ATTR_TYPE_UINT16:
        case MIB_ATTR_TYPE_UINT32:
        case MIB_ATTR_TYPE_UINT64:
        case MIB_ATTR_TYPE_TABLE:
        {
            char *pch = NULL, byte[3];
            int i = 0;
            //UINT8 value[attrLen];
            UINT8 *pValue = NULL;
            pValue = (UINT8 *)malloc(sizeof(UINT8) * attrLen);
            if (!pValue) return GOS_FAIL;
            pch = (char *)val;
            for(i = 0; i < attrLen; i++)
            {
                memset(byte, '\0', 3);
                snprintf(byte, 3, "%c%c", *pch, *(pch + 1));
                //value[i] = strtol(byte, NULL, 16);
                *(pValue + i) = strtol(byte, NULL, 16);
                pch += 2;
            }

            MIB_SetAttrToBuf(tableId, attrIndex, pValue, pMibRowBuff, attrLen);
            free(pValue);
            break;
        }
        case MIB_ATTR_TYPE_STR:
            MIB_SetAttrToBuf(tableId, attrIndex, val, pMibRowBuff, attrLen+1);
            break;
        default:
            break;
    }

    // Save in MIB
    if (GOS_OK != MIB_SetAttributes(tableId, pMibRowBuff, entrySize, &validMibSet))
    {
        OMCI_PRINT("MIB Set: error, class=%d  <--> tableId=%u, entity=0x%x, attrSet=0x%x", classId, tableId, entityId, validMibSet);
        return GOS_FAIL;
    }


    if (GOS_OK != MIB_Get(tableId, pMibRowBuff, entrySize))
    {
        OMCI_PRINT("SingleSet: entity not exist in MIB, class=%d, entity=0x%x", classId, entityId);
        return GOS_FAIL;
    }

    if (GOS_OK != OMCI_MeOperCfg(tableId, oldMibRow, pMibRowBuff, MIB_SET, validMibSet, OMCI_MSG_BASELINE_PRI_LOW))
    {
        OMCI_PRINT("SingleSet: SingleSet error, class=%d, entity=0x%x, attrSet=0x%x", classId, entityId, validMibSet);
        // Rollback MIB
        MIB_Set(tableId, oldMibRow, entrySize);
        return GOS_FAIL;
    }
    return GOS_OK;
}

GOS_ERROR_CODE OMCI_LoidSet_Cmd(unsigned char *loid, unsigned char *passwd)
{

    MIB_TABLE_ONTG_T ontg, oldOntg;
    MIB_TABLE_LOIDAUTH_T loidAuth, oldLoidAuth;
    MIB_ATTRS_SET mibAttrsSet;

    memcpy(gInfo.loidCfg.loid, loid, sizeof(gInfo.loidCfg.loid));
    memcpy(gInfo.loidCfg.loidPwd, passwd, sizeof(gInfo.loidCfg.loidPwd));

    /*Set to ONTG (G.988)*/
    if(MIB_GetFirst(MIB_TABLE_ONTG_INDEX,&ontg,sizeof(MIB_TABLE_ONTG_T))!=GOS_OK)
    {
        return GOS_FAIL;
    }
    memcpy(&oldOntg, &ontg, sizeof(MIB_TABLE_ONTG_T));

    memcpy(ontg.LogicalOnuID, loid, sizeof(ontg.LogicalOnuID)-1);
    memcpy(ontg.LogicalPassword, passwd, sizeof(ontg.LogicalPassword)-1);
    if(MIB_Set(MIB_TABLE_ONTG_INDEX,&ontg,sizeof(MIB_TABLE_ONTG_T))!=GOS_OK)
    {
        return GOS_FAIL;
    }

    MIB_ClearAttrSet(&mibAttrsSet);
    MIB_SetAttrSet(&mibAttrsSet, MIB_TABLE_ONTG_LOGICAL_ONU_ID_INDEX);
    MIB_SetAttrSet(&mibAttrsSet, MIB_TABLE_ONTG_LOGICAL_PASSWORD_INDEX);
    if (GOS_OK != OMCI_MeOperCfg(MIB_TABLE_ONTG_INDEX, &oldOntg, &ontg, MIB_SET, mibAttrsSet, OMCI_MSG_BASELINE_PRI_LOW))
    {
        OMCI_PRINT("OMCI_LoidSet ONT-G OMCI_MeOperCfg error");
        return GOS_FAIL;
    }


    /*Also set to LOID ME*/
    if(MIB_GetFirst(MIB_TABLE_LOIDAUTH_INDEX,&loidAuth,sizeof(MIB_TABLE_LOIDAUTH_T))!=GOS_OK)
    {
        return GOS_FAIL;
    }
    memcpy(&oldLoidAuth, &loidAuth, sizeof(MIB_TABLE_LOIDAUTH_T));

    memcpy(loidAuth.LoID, loid, sizeof(loidAuth.LoID)-1);
    memcpy(loidAuth.Password, passwd, sizeof(loidAuth.Password)-1);
    if(MIB_Set(MIB_TABLE_LOIDAUTH_INDEX,&loidAuth,sizeof(MIB_TABLE_LOIDAUTH_T))!=GOS_OK)
    {
        return GOS_FAIL;
    }

    MIB_ClearAttrSet(&mibAttrsSet);
    MIB_SetAttrSet(&mibAttrsSet, MIB_TABLE_LOIDAUTH_LOID_INDEX);
    MIB_SetAttrSet(&mibAttrsSet, MIB_TABLE_LOIDAUTH_PASSWORD_INDEX);
    if (GOS_OK != OMCI_MeOperCfg(MIB_TABLE_LOIDAUTH_INDEX, &oldLoidAuth, &loidAuth, MIB_SET, mibAttrsSet, OMCI_MSG_BASELINE_PRI_LOW))
    {
        OMCI_PRINT("OMCI_LoidSet LOID OMCI_MeOperCfg error");
        return GOS_FAIL;
    }

    return GOS_OK;
}


GOS_ERROR_CODE OMCI_LoidGet_Cmd(void)
{
    int oldstdout = -1, nul = -1;
    const int stdout_fd = fileno(stdout);
    nul = omci_open_cli_fd();

    if (-1 != nul)
    {
        oldstdout = dup(stdout_fd);
        dup2(nul, stdout_fd);
        close(nul);
    }

    OMCI_PRINT("LOID   : %s", gInfo.loidCfg.loid);
    OMCI_PRINT("LOIDPWD: %s", gInfo.loidCfg.loidPwd);
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

GOS_ERROR_CODE OMCI_LoidAuthStatusGet_Cmd(UINT32 srcMsgQkey)
{
    PON_OMCI_CMD_T msg;

    /*Get LOID last Auth Status set by OLT*/
    /*Send Response to MsgQ*/
    memset(&msg,0,sizeof(PON_OMCI_CMD_T));
    msg.cmd = PON_OMCI_CMD_LOIDAUTH_GET_RSP;
    msg.state =  gInfo.loidCfg.lastLoidAuthStatus;
    OMCI_SendMsg(srcMsgQkey, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

GOS_ERROR_CODE OMCI_LoidAuthNumGet_Cmd ( UINT32 srcMsgQkey )
{
    PON_OMCI_CMD_T msg;

    /*Get LOID Auth Num*/
    /*Send Response to MsgQ*/
    memset(&msg,0,sizeof(PON_OMCI_CMD_T));
    msg.cmd = PON_OMCI_CMD_LOIDAUTH_NUM_GET_RSP;
    msg.state =  gInfo.loidCfg.loidAuthNum;
    msg.level =  gInfo.loidCfg.loidAuthSuccessNum;
    OMCI_SendMsg(srcMsgQkey, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}


GOS_ERROR_CODE OMCI_LoidAuthNumReset_Cmd(UINT32 srcMsgQkey)
{
    /*Reset LOID Auth Num*/
    gInfo.loidCfg.loidAuthNum = 0;
    gInfo.loidCfg.loidAuthSuccessNum = 0;

    return GOS_OK;
}


GOS_ERROR_CODE
OMCI_IphostDhcpSet_Cmd (
    if_info_t* dhcpInfo)
{
    MIB_TABLE_IP_HOST_CFG_DATA_T oldIphost, iphost;

    if (!dhcpInfo)
    {
        return GOS_ERR_PARAM;
    }

    OMCI_LOG (
        OMCI_LOG_LEVEL_INFO,
        "%s, iphostId 0x%X, ip %x\n",
        __FUNCTION__,
        dhcpInfo->if_id,
        dhcpInfo->ip_addr.ipv4_addr);

    iphost.EntityID = dhcpInfo->if_id;
    if (GOS_OK != MIB_Get(
                        MIB_TABLE_IP_HOST_CFG_DATA_INDEX,
                        &iphost,
                        sizeof(MIB_TABLE_IP_HOST_CFG_DATA_T)))
    {
        OMCI_LOG(
            OMCI_LOG_LEVEL_ERR,
            "%s: Can't get the IPHOST[EntityID: 0x%x]\n",
            __FUNCTION__,
            iphost.EntityID);
        return GOS_ERR_NOT_FOUND;
    }
#if 0 // for Yueme, tr69 wan created by default

    if (iphost.IpOptions^IP_HOST_CFG_DATA_IP_OPTIONS_ENABLE_DHCP ||
            !dhcpInfo->if_is_DHCP_B)
    {
        OMCI_LOG(
            OMCI_LOG_LEVEL_ERR,
            "%s: DHCP is disabled\n",
            __FUNCTION__);
        return GOS_ERR_DISABLE;
    }
#endif
    memcpy(&oldIphost, &iphost, sizeof(MIB_TABLE_IP_HOST_CFG_DATA_T));

    iphost.CurrentAddress = dhcpInfo->ip_addr.ipv4_addr;
    iphost.CurrentMask    = dhcpInfo->mask_addr.ipv4_mask_addr;
    iphost.CurrentGateway      = dhcpInfo->gateway_addr.ipv4_gateway_addr;
    iphost.CurrentPrimaryDns   = dhcpInfo->primary_dns_addr.ipv4_primary_dns_addr;
    iphost.CurrentSecondaryDns = dhcpInfo->second_dns_addr.ipv4_second_dns_addr;

    if (MIB_Set(
            MIB_TABLE_IP_HOST_CFG_DATA_INDEX,
            &iphost,
            sizeof(MIB_TABLE_IP_HOST_CFG_DATA_T)) != GOS_OK)
    {
        OMCI_LOG(
            OMCI_LOG_LEVEL_ERR,
            "%s: Failed to set the IPHOST MIB\n",
            __FUNCTION__);
        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE OMCI_AuthUptimeGet_Cmd(UINT32 srcMsgQkey)
{
    PON_OMCI_CMD_T msg;

    /*Send Response to MsgQ*/
    memset(&msg, 0, sizeof(PON_OMCI_CMD_T));
    msg.cmd         = PON_OMCI_CMD_DURATION_GET_RSP;
    clock_gettime(CLOCK_MONOTONIC, &gInfo.adtInfo.end_time);
    msg.difftime    = difftime(gInfo.adtInfo.end_time.tv_sec, gInfo.adtInfo.start_time.tv_sec);
    OMCI_SendMsg(srcMsgQkey, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

GOS_ERROR_CODE OMCI_DrvVersionGet_Cmd(void)
{

#ifdef CONFIG_RELEASE_VERSION

    char drvVersion[64]="";

    omci_wrapper_getDrvVersion(drvVersion);

    int oldstdout = -1, nul = -1;
    const int stdout_fd = fileno(stdout);
    nul = omci_open_cli_fd();

    if (-1 != nul)
    {
        oldstdout = dup(stdout_fd);
        dup2(nul, stdout_fd);
        close(nul);
    }

    OMCI_PRINT("OMCI DRV version : %s", drvVersion);

    if (-1 != nul)
    {
        fflush(stdout);
        if (-1 != oldstdout)
        {
            dup2(oldstdout, stdout_fd);
            close(oldstdout);
        }
    }

#endif

    return GOS_OK;
}


GOS_ERROR_CODE OMCI_OltLocationGet_Cmd(UINT32 srcMsgQkey)
{
    PON_OMCI_CMD_T msg;
    MIB_TABLE_OLTLOCATIONCFGDATA_T mibOltLocation;

    /*Get OLT Location cfg data*/
    memset(&msg,0,sizeof(PON_OMCI_CMD_T));
    msg.cmd = PON_OMCI_CMD_OLT_LOCATION_GET_RSP;

    if(MIB_GetFirst(MIB_TABLE_OLT_LOCATION_CFG_DATA_INDEX, &mibOltLocation, sizeof(MIB_TABLE_OLTLOCATIONCFGDATA_T))!=GOS_OK)
    {
        return GOS_FAIL;
    }
    memcpy(&msg.value[0], &mibOltLocation.Longitude, sizeof(MIB_TABLE_OLTLOCATIONCFGDATA_T) - 2);

    msg.state = gInfo.oltLocationState;

    /*Send Response to MsgQ*/
    OMCI_SendMsg(srcMsgQkey, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, &msg, sizeof(PON_OMCI_CMD_T));

    return GOS_OK;
}

GOS_ERROR_CODE omci_cmd_set_dmmode_handler(int state)
{
    gInfo.dmMode = state;

    omci_wrapper_setDualMgmtMode(gInfo.dmMode);

    return GOS_OK;
}

GOS_ERROR_CODE omci_cmd_get_dmmode_handler(void)
{
    int oldstdout = -1, nul = -1;
    const int stdout_fd = fileno(stdout);
    char *dm_mode[] = {"off", "wan_queue", "bc_mc", "wan_queue and bc_mc"};

    nul = omci_open_cli_fd();
    if (-1 != nul)
    {
        oldstdout = dup(stdout_fd);
        dup2(nul, stdout_fd);
        close(nul);
    }

    OMCI_PRINT("dmMode = %s", dm_mode[gInfo.dmMode]);
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

GOS_ERROR_CODE omci_cmd_send_alarm_handler(int type, unsigned int number, int status, unsigned short detail)
{
    omci_alm_t alarmMsg;

    alarmMsg.almType = type;
    alarmMsg.almData.almNumber = number;
    alarmMsg.almData.almStatus = status;
    alarmMsg.almData.almDetail = detail;

    return omci_ext_alarm_dispatcher(&alarmMsg);
}

void omci_dump_alarm_by_alarm_table(omci_me_instance_t instanceId, mib_alarm_table_t *pAlarmTable)
{
    UINT8   i, j;

    if (!pAlarmTable)
        return;

    for (i = 0; i < OMCI_ALARM_NUMBER_BITMASK_IN_BYTE; i++)
    {
        for (j = 0; j < 8; j++)
        {
            if ((0x80 >> j) & pAlarmTable->aBitMask[i])
            {
                OMCI_PRINT("Instance %#x Number %d is declared\n", instanceId, i * 8 + j);
            }
        }
    }
}

void omci_dump_alarm_by_me_instance(MIB_TABLE_T *pTable, unsigned short instanceId)
{
    MIB_ENTRY_T         *pEntry;
    MIB_TABLE_INDEX     tableIndex;
    omci_me_instance_t  tempInstanceId;

    if (!pTable || 0 == pTable->curEntryCount)
        return;

    tableIndex = pTable->tableIndex;

    OMCI_PRINT("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
    OMCI_PRINT("%s", MIB_GetTableName(tableIndex));
    OMCI_PRINT("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");

    if (USHRT_MAX == instanceId)
    {
        LIST_FOREACH(pEntry, &pTable->entryHead, entries)
        {
            tempInstanceId = *(omci_me_instance_t *)pEntry->pData;

            omci_dump_alarm_by_alarm_table(tempInstanceId, (mib_alarm_table_t *)pEntry->pAlarmTable);
        }
    }
    else
    {
        LIST_FOREACH(pEntry, &pTable->entryHead, entries)
        {
            tempInstanceId = *(omci_me_instance_t *)pEntry->pData;
            if (tempInstanceId != instanceId)
                continue;

            omci_dump_alarm_by_alarm_table(tempInstanceId, (mib_alarm_table_t *)pEntry->pAlarmTable);

            break;
        }
    }
}

GOS_ERROR_CODE omci_cmd_dump_alarm_handler(int meId, unsigned short instanceId)
{
    MIB_TABLE_T         *pTable;
    MIB_TABLE_INDEX     tableIndex;
    GOS_ERROR_CODE      ret = GOS_ERR_PARAM;
    int oldstdout = -1, nul = -1;
    const int stdout_fd = fileno(stdout);
    nul = omci_open_cli_fd();

    if (-1 != nul)
    {
        oldstdout = dup(stdout_fd);
        dup2(nul, stdout_fd);
        close(nul);
    }

    if (INT_MAX == meId)
    {
        for (tableIndex = MIB_TABLE_FIRST_INDEX;
                tableIndex <= MIB_TABLE_LAST_INDEX; tableIndex = MIB_TABLE_NEXT_INDEX(tableIndex))
        {
            pTable = mib_GetTablePtr(tableIndex);
            if (!pTable)
                goto dump_alarm_done;

            omci_dump_alarm_by_me_instance(pTable, USHRT_MAX);
        }
    }
    else
    {
        tableIndex = MIB_GetTableIndexByClassId(meId);
        pTable = mib_GetTablePtr(tableIndex);
        if (!pTable)
            goto dump_alarm_done;

        omci_dump_alarm_by_me_instance(pTable, instanceId);
    }
    ret = GOS_OK;
dump_alarm_done:

    if (-1 != nul)
    {
        fflush(stdout);
        if (-1 != oldstdout)
        {
            dup2(oldstdout, stdout_fd);
            close(oldstdout);
        }
    }
    return ret;
}

#if 0
static BOOL omci_IsDiffDscpRule(omci_vlan_rule_t *pNew, omci_vlan_rule_t *pOld)
{
    if (!pNew || !pOld)
        return FALSE;

    if((PRI_ACT_FROM_DSCP == pNew->vlanRule.sTagAct.priAct)&&
        (PRI_ACT_FROM_DSCP == pOld->vlanRule.sTagAct.priAct))
    {
        if(pNew->vlanRule.sTagAct.assignVlan.pri != pOld->vlanRule.sTagAct.assignVlan.pri)
        {
            printf("S pri %d %d\r\n",pNew->vlanRule.sTagAct.assignVlan.pri,pOld->vlanRule.sTagAct.assignVlan.pri);
            return TRUE;
        }
    }
    if((PRI_ACT_FROM_DSCP == pNew->vlanRule.cTagAct.priAct)&&
        (PRI_ACT_FROM_DSCP == pOld->vlanRule.cTagAct.priAct))
    {
        if(pNew->vlanRule.cTagAct.assignVlan.pri != pOld->vlanRule.cTagAct.assignVlan.pri)
        {
            printf("C pri %d %d\r\n",pNew->vlanRule.cTagAct.assignVlan.pri,pOld->vlanRule.cTagAct.assignVlan.pri);
            return TRUE;
        }
    }
    return FALSE;

}
#endif

static BOOL omci_IsIgnoreDsRule(omci_vlan_rule_t *pNew, omci_vlan_rule_t *pOld)
{
    if (!pNew || !pOld)
        return TRUE;

    if (!memcmp(&(pNew->vlanRule.filterRule), &(pOld->vlanRule.filterRule), sizeof(OMCI_VLAN_FILTER_ts)) &&
        (( ((VLAN_ACT_NON == pNew->vlanRule.sTagAct.vlanAct && VLAN_ACT_TRANSPARENT == pOld->vlanRule.sTagAct.vlanAct) ||
           (VLAN_ACT_TRANSPARENT == pNew->vlanRule.sTagAct.vlanAct && VLAN_ACT_NON == pOld->vlanRule.sTagAct.vlanAct)) &&
           (!memcmp(&pNew->vlanRule.cTagAct, &pOld->vlanRule.cTagAct, sizeof(OMCI_VLAN_ACT_ts)))  ) ||
         ( ((VLAN_ACT_NON == pNew->vlanRule.cTagAct.vlanAct && VLAN_ACT_TRANSPARENT == pOld->vlanRule.cTagAct.vlanAct) ||
           (VLAN_ACT_TRANSPARENT == pNew->vlanRule.cTagAct.vlanAct && VLAN_ACT_NON == pOld->vlanRule.cTagAct.vlanAct)) &&
           (!memcmp(&pNew->vlanRule.sTagAct, &pOld->vlanRule.sTagAct, sizeof(OMCI_VLAN_ACT_ts)))  )) )
    {
        return TRUE;
    }
    return FALSE;
}

/*Traffic related*/
static GOS_ERROR_CODE omci_AddTrafficRule(MIB_TREE_CONN_T *pConn,int index,omci_vlan_rule_t *pRule)
{
    omci_vlan_rule_t *pEntry;
    /*check filter rule is the same*/
    LIST_FOREACH(pEntry,&pConn->ruleHead[index],entries)
    {
        if (pEntry->vlanRule.filterMode == pRule->vlanRule.filterMode)
        {
            if (pRule->dir != PON_GEMPORT_DIRECTION_DS)
            {
                if (!memcmp(&pRule->vlanRule.filterRule, &pEntry->vlanRule.filterRule, sizeof(OMCI_VLAN_FILTER_ts)))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                        "%s: Add Traffic Data/Mcast Rule failed, has the same rule!",__FUNCTION__);
                    return GOS_FAIL;

                }
            }
            else
            {
                if ((!memcmp(&pRule->vlanRule.filterRule, &pEntry->vlanRule.filterRule, sizeof(OMCI_VLAN_FILTER_ts))) ||
                    omci_IsIgnoreDsRule(pRule, pEntry))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                        "%s: Add Traffic Mcast Rule failed, has the same rule!",__FUNCTION__);
                    return GOS_FAIL;
                }
            }
        }
    }
    if (pConn->pMcastGemIwTp)
        pRule->vlanRule.outStyle.isMCRule = TRUE;

    if (pConn->pUniExtVlanCfg &&
        LIST_EMPTY(&pConn->pUniExtVlanCfg->head) &&
        0x88A8 == pConn->pUniExtVlanCfg->OutputTPID)
    {
        if (VLAN_OPER_MODE_FILTER_SINGLETAG == pRule->vlanRule.filterMode &&
            VLAN_FILTER_NO_CARE_TAG == pRule->vlanRule.filterRule.filterStagMode &&
            (VLAN_FILTER_VID & pRule->vlanRule.filterRule.filterCtagMode ||
            VLAN_FILTER_TCI & pRule->vlanRule.filterRule.filterCtagMode ||
            VLAN_FILTER_PRI & pRule->vlanRule.filterRule.filterCtagMode) &&
            VLAN_ACT_TRANSPARENT == pRule->vlanRule.sTagAct.vlanAct &&
            VLAN_ACT_TRANSPARENT == pRule->vlanRule.cTagAct.vlanAct &&
            1 == pRule->vlanRule.outStyle.outTagNum)
        {
            pRule->vlanRule.outStyle.tpid = TPID_88A8;
        }
    }

    LIST_INSERT_HEAD(&pConn->ruleHead[index], pRule, entries);
    return GOS_OK;
}

static GOS_ERROR_CODE
isSameAniVid(aclTableEntry_t *pEntry, struct aclHead *pAclHead)
{
    aclTableEntry_t *ptr = NULL;
    LIST_FOREACH(ptr, pAclHead, entries)
    {
        /*if(1023 == ptr->tableEntry.tableCtrl.bit.rowKey)
            continue;*/
        if(ptr->tableEntry.tableCtrl.bit.rowKey < pEntry->tableEntry.tableCtrl.bit.rowKey &&
            ROW_PART_0 == ptr->tableEntry.tableCtrl.bit.rowPartId)
        {
            if(pEntry->tableEntry.rowPart.rowPart0.aniVid == ptr->tableEntry.rowPart.rowPart0.aniVid)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

static void
omci_GetVlanActByStagAct(
    omci_vlan_rule_t *ptr, OMCI_VLAN_ACT_ts **ppTagAct, OMCI_VLAN_ts **ppVlan)
{
    switch (ptr->vlanRule.sTagAct.vlanAct)
    {
        case VLAN_ACT_ADD:
        case VLAN_ACT_MODIFY:
            (*ppTagAct) = &(ptr->vlanRule.sTagAct);
            (*ppVlan) = &(ptr->vlanRule.sTagAct.assignVlan);
            break;
        default:
            (*ppTagAct) = &(ptr->vlanRule.cTagAct);
            (*ppVlan) = &(ptr->vlanRule.cTagAct.assignVlan);
    }
}

static void
processDsIgmpMcastTci(mopTableEntry_t *pMopTblEntry, UINT8 *pDsTci,
    MIB_TREE_CONN_T *pConn, int index, omci_vlan_rule_t *ptr)
{
    UINT16 dsIgmpMcastPri;
    UINT16 dsIgmpMcastVid;
    UINT32 vlanAct, priAct;
    UINT16 pri;
    OMCI_VLAN_ACT_ts *pTagAct;
    OMCI_VLAN_ts *pVlan;

    if(!ptr || !pDsTci)
        return;

    dsIgmpMcastPri = (pDsTci[1] >> 5);
    dsIgmpMcastVid = (((pDsTci[1] & 0x0F) << 8) | pDsTci[2]);

    omci_GetVlanActByStagAct(ptr, &pTagAct, &pVlan);

    switch (pDsTci[0])
    {
        case PASS:
            break;
        case STRIP_TAG:
            memset(&ptr->vlanRule.sTagAct, 0, sizeof(OMCI_VLAN_ACT_ts));
            memset(&ptr->vlanRule.cTagAct, 0, sizeof(OMCI_VLAN_ACT_ts));
            ptr->vlanRule.sTagAct.vlanAct = VLAN_ACT_REMOVE;
            ptr->vlanRule.cTagAct.vlanAct = VLAN_ACT_REMOVE;
            ptr->vlanRule.outStyle.outTagNum = 0;
            memset(&ptr->vlanRule.outStyle.outVlan, 0, sizeof(OMCI_VLAN_ts));
            break;

        case ADD_TAG:
        case REPLACE_TAG:
        case REPLACE_VID:
            vlanAct = (REPLACE_VID == pDsTci[0] ? VLAN_ACT_MODIFY : VLAN_ACT_ADD);
            priAct = (REPLACE_VID == pDsTci[0] ? PRI_ACT_COPY_OUTER : PRI_ACT_ASSIGN);
            pri = (REPLACE_VID == pDsTci[0] ?
                ptr->vlanRule.sTagAct.assignVlan.pri : dsIgmpMcastPri);

            pTagAct->vlanAct = vlanAct;
            pTagAct->vidAct = VID_ACT_ASSIGN;
            pTagAct->priAct = priAct;
            pTagAct->assignVlan.pri = pri;
            pTagAct->assignVlan.vid = dsIgmpMcastVid;

            memcpy(&(ptr->vlanRule.outStyle.outVlan), pVlan, sizeof(OMCI_VLAN_ts));
            if (0 == ptr->vlanRule.outStyle.outTagNum)
                ptr->vlanRule.outStyle.outTagNum = 1;

            break;

        case ADD_VIDUNI_TAG:
            /* Unspecified and untag: no change  dsIgmpMcastPri and dsIgmpMcastVid */
            //TBD: Only process vid and don't care priority, since 988 spec not defined it
            dsIgmpMcastPri = ((USHRT_MAX == pMopTblEntry->tableEntry.vidUni ||
                                    4096 == pMopTblEntry->tableEntry.vidUni) ?
                                    dsIgmpMcastPri : ((pMopTblEntry->tableEntry.vidUni) >> 13));
            dsIgmpMcastVid = ((USHRT_MAX == pMopTblEntry->tableEntry.vidUni ||
                                    4096 == pMopTblEntry->tableEntry.vidUni) ?
                                    dsIgmpMcastVid : ((pMopTblEntry->tableEntry.vidUni) & 0xFFF));

            pTagAct->vlanAct = VLAN_ACT_ADD;
            pTagAct->vidAct = VID_ACT_ASSIGN;
            pTagAct->priAct = PRI_ACT_ASSIGN;
            pTagAct->assignVlan.pri = dsIgmpMcastPri;
            pTagAct->assignVlan.vid = dsIgmpMcastVid;
            memcpy(&(ptr->vlanRule.outStyle.outVlan), pVlan, sizeof(OMCI_VLAN_ts));
            break;

        case REPLACE_VIDUNI_TAG:
        case REPLACE_VID2VIDUNI:
            if(4096 == pMopTblEntry->tableEntry.vidUni)
            {
                pTagAct->vlanAct = VLAN_ACT_REMOVE;
            }
            else
            {
                if(USHRT_MAX != pMopTblEntry->tableEntry.vidUni)
                {
                    dsIgmpMcastPri = (pMopTblEntry->tableEntry.vidUni) >> 13;
                    dsIgmpMcastVid = (pMopTblEntry->tableEntry.vidUni) & 0xFFF;
                }

                priAct = (REPLACE_VID2VIDUNI == pDsTci[0] ? PRI_ACT_TRANSPARENT : PRI_ACT_ASSIGN);
                pri = (REPLACE_VID2VIDUNI == pDsTci[0] ?
                    ptr->vlanRule.sTagAct.assignVlan.pri : dsIgmpMcastPri);

                pTagAct->vlanAct = VLAN_ACT_MODIFY;
                pTagAct->vidAct = VID_ACT_ASSIGN;
                pTagAct->priAct = priAct;
                pTagAct->assignVlan.pri = pri;
                pTagAct->assignVlan.vid = dsIgmpMcastVid;
                memcpy(&(ptr->vlanRule.outStyle.outVlan), pVlan, sizeof(OMCI_VLAN_ts));
            }
            break;

        default:
            break;
    }
    return;
}

static GOS_ERROR_CODE
omci_check_valid_rule(MIB_TREE_CONN_T *pConn, omci_vlan_rule_t *pTrafficRule, UINT16 *pAniVid)
{
    BOOL                                is_valid = FALSE;
    UINT16                              me309_ani_vid = 0xFFFF;

    OMCI_ERR_CHK(OMCI_LOG_LEVEL_DBG, (!pConn->pUniExtVlanCfg), GOS_OK);

    if (pAniVid)
        me309_ani_vid = *pAniVid;

    //no any filter vlan, so each ext vlan rule is valid.
    if (!pConn->pAniVlanTagFilter && 0xFFFF == me309_ani_vid)
    {
        // ignore inverse of C+F->F and S+C+F->F
        if (VLAN_FILTER_NO_TAG & pTrafficRule->vlanRule.filterRule.filterStagMode &&
            VLAN_FILTER_NO_TAG & pTrafficRule->vlanRule.filterRule.filterCtagMode &&
            (EVTOCD_TBL_TREATMENT_VID_COPY_FROM_INNER == pTrafficRule->vlanRule.sTagAct.assignVlan.vid ||
            EVTOCD_TBL_TREATMENT_VID_COPY_FROM_INNER == pTrafficRule->vlanRule.cTagAct.assignVlan.vid))
        {
            return GOS_FAIL;
        }
        return GOS_OK;
    }
    // check mcast gem with vlan filter or not
    if (!pConn->pAniVlanTagFilter && 0xFFFF != me309_ani_vid)
    {
        // select the corresponding action rule by searching treat vlan id = me309 ani vid
        if (VLAN_FILTER_VID & pTrafficRule->vlanRule.filterRule.filterStagMode ||
            VLAN_FILTER_TCI & pTrafficRule->vlanRule.filterRule.filterStagMode)
        {
            if (pTrafficRule->vlanRule.filterRule.filterSTag.vid  == me309_ani_vid)
                is_valid = TRUE;
        }
        else if (VLAN_FILTER_VID & pTrafficRule->vlanRule.filterRule.filterCtagMode ||
            VLAN_FILTER_TCI & pTrafficRule->vlanRule.filterRule.filterCtagMode)
        {
            if (pTrafficRule->vlanRule.filterRule.filterCTag.vid  == me309_ani_vid)
                is_valid = TRUE;
        }
    }
    else
    {
        is_valid = TRUE;
    }

    if (is_valid)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "%s@%d, svid=%u, cvid=%u",
            __FUNCTION__, __LINE__, pTrafficRule->vlanRule.filterRule.filterSTag.vid,
            pTrafficRule->vlanRule.filterRule.filterCTag.vid);
        return GOS_OK;
    }
    return GOS_FAIL;
}

static GOS_ERROR_CODE
omci_AddMcastTrafficRuleByAcl(mopTableEntry_t *pMopTblEntry, struct aclHead *pAclHead,
    MIB_TREE_CONN_T *pConn, int index, omci_vlan_rule_t *pTrafficRule, UINT8 *pDsTci)
{
    aclTableEntry_t *pEntry = NULL;
    omci_vlan_rule_t *p_new_vlan_rule = NULL;

    LIST_FOREACH(pEntry, pAclHead, entries)
    {
        if(1023 == pEntry->tableEntry.tableCtrl.bit.rowKey ||
            ROW_PART_0 != pEntry->tableEntry.tableCtrl.bit.rowPartId)
            continue;

        if(ROW_PART_0 != pEntry->tableEntry.tableCtrl.bit.rowPartId)
            continue;

        if(isSameAniVid(pEntry, pAclHead))
            continue;

        p_new_vlan_rule = (omci_vlan_rule_t*)malloc(sizeof(omci_vlan_rule_t));

        OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (!p_new_vlan_rule), GOS_FAIL);

        memcpy(p_new_vlan_rule, pTrafficRule, sizeof(omci_vlan_rule_t));

        p_new_vlan_rule->vlanRule.filterRule.etherType = ETHTYPE_FILTER_NO_CARE;


        switch (pTrafficRule->vlanRule.filterMode)
        {
            case VLAN_OPER_MODE_FORWARD_ALL:
            case VLAN_OPER_MODE_FILTER_SINGLETAG:
            case VLAN_OPER_MODE_EXTVLAN:
                if (VLAN_OPER_MODE_FORWARD_ALL == pTrafficRule->vlanRule.filterMode)
                {
                    if (0 != pEntry->tableEntry.rowPart.rowPart0.aniVid)
                    {
                        p_new_vlan_rule->vlanRule.filterMode = VLAN_OPER_MODE_FILTER_SINGLETAG;
                        pTrafficRule->vlanRule.outStyle.tpid = TPID_8100;
                    }
                    else
                    {
                        p_new_vlan_rule->vlanRule.filterMode = VLAN_OPER_MODE_FORWARD_UNTAG;
                        pTrafficRule->vlanRule.outStyle.tpid = TPID_0800;
                    }
                }

                if (TPID_8100 == pTrafficRule->vlanRule.outStyle.tpid)
                {
                    memset(&p_new_vlan_rule->vlanRule.filterRule.filterSTag, 0, sizeof(OMCI_VLAN_ts));
                    p_new_vlan_rule->vlanRule.filterRule.filterStagMode = VLAN_FILTER_NO_TAG;
                    memset(&p_new_vlan_rule->vlanRule.filterRule.filterCTag, 0, sizeof(OMCI_VLAN_ts));
                    p_new_vlan_rule->vlanRule.filterRule.filterCtagMode = VLAN_FILTER_VID;
                    p_new_vlan_rule->vlanRule.filterRule.filterCTag.vid =
                        pEntry->tableEntry.rowPart.rowPart0.aniVid;
                }
                else if (TPID_0800 == pTrafficRule->vlanRule.outStyle.tpid)
                {
                    //nothing
                }
                else
                {
                    memset(&p_new_vlan_rule->vlanRule.filterRule.filterCTag, 0, sizeof(OMCI_VLAN_ts));
                    p_new_vlan_rule->vlanRule.filterRule.filterCtagMode = VLAN_FILTER_NO_TAG;
                    memset(&p_new_vlan_rule->vlanRule.filterRule.filterSTag, 0, sizeof(OMCI_VLAN_ts));
                    p_new_vlan_rule->vlanRule.filterRule.filterStagMode = VLAN_FILTER_VID;
                    p_new_vlan_rule->vlanRule.filterRule.filterSTag.vid =
                        pEntry->tableEntry.rowPart.rowPart0.aniVid;
                }

                if (PASS == pDsTci[0] && VLAN_OPER_MODE_EXTVLAN == pTrafficRule->vlanRule.filterMode)
                {
                    if (GOS_FAIL == omci_check_valid_rule(pConn, pTrafficRule,
                        &pEntry->tableEntry.rowPart.rowPart0.aniVid))
                    {
                        free(p_new_vlan_rule);
                        break;
                    }
                }

                processDsIgmpMcastTci(pMopTblEntry, pDsTci, pConn, index, p_new_vlan_rule);

                if(GOS_OK != omci_AddTrafficRule(pConn, index, p_new_vlan_rule))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "add traffic rule failed\n");
                    free(p_new_vlan_rule);
                    continue;
                }

                OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                    "mcast gem port id=%u, uni mbpcd=%x, srvId=%u, vid=%u\n",
                    p_new_vlan_rule->outgress, ((NULL != pConn->pUniPort) ? pConn->pUniPort->EntityID : 0x0),
                    p_new_vlan_rule->servId, p_new_vlan_rule->vlanRule.filterRule.filterCTag.vid);
                break;
            default:
                free(p_new_vlan_rule);
                continue;
        }
    }
    return GOS_OK;
}

static GOS_ERROR_CODE
omci_AddMcastTrafficRuleByNoAcl(mopTableEntry_t *pMopTblEntry,
    MIB_TREE_CONN_T *pConn, int index, omci_vlan_rule_t *pTrafficRule, UINT8 *pDsTci)
{
    omci_vlan_rule_t *p_new_vlan_rule = NULL;
    unsigned int cvlan_state;

    OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (GOS_OK != omci_wrapper_getCvlanState(&cvlan_state)), GOS_FAIL);
    p_new_vlan_rule = (omci_vlan_rule_t*)malloc(sizeof(omci_vlan_rule_t));
    OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (!p_new_vlan_rule), GOS_FAIL);

    memcpy(p_new_vlan_rule, pTrafficRule, sizeof(omci_vlan_rule_t));

    if (cvlan_state)
        p_new_vlan_rule->isLatchB = TRUE;
    p_new_vlan_rule->vlanRule.filterRule.etherType = ETHTYPE_FILTER_NO_CARE;

    switch (pTrafficRule->vlanRule.filterMode)
    {
        case VLAN_OPER_MODE_FORWARD_ALL:
            break;

        case VLAN_OPER_MODE_FORWARD_UNTAG:
        case VLAN_OPER_MODE_FORWARD_SINGLETAG:
        case VLAN_OPER_MODE_FILTER_INNER_PRI:
            break;

        case VLAN_OPER_MODE_FILTER_SINGLETAG:
        case VLAN_OPER_MODE_EXTVLAN:
            if (cvlan_state)
            {
                memset(&p_new_vlan_rule->vlanRule.filterRule.filterSTag, 0, sizeof(OMCI_VLAN_ts));
                p_new_vlan_rule->vlanRule.filterRule.filterStagMode = VLAN_FILTER_NO_TAG;
                memset(&p_new_vlan_rule->vlanRule.filterRule.filterCTag, 0, sizeof(OMCI_VLAN_ts));
                p_new_vlan_rule->vlanRule.filterRule.filterCtagMode = VLAN_FILTER_VID;
                p_new_vlan_rule->vlanRule.filterRule.filterCTag.vid = 1;
            }
            /*else
            {
                if (!pConn->pAniVlanTagFilter && !pConn->pUniVlanTagFilter)
                {
                    memset(&p_new_vlan_rule->vlanRule.filterRule.filterSTag, 0, sizeof(OMCI_VLAN_ts));
                    memset(&p_new_vlan_rule->vlanRule.filterRule.filterCTag, 0, sizeof(OMCI_VLAN_ts));
                    p_new_vlan_rule->vlanRule.filterRule.filterStagMode = VLAN_FILTER_NO_CARE_TAG;
                    p_new_vlan_rule->vlanRule.filterRule.filterCtagMode = VLAN_FILTER_NO_CARE_TAG;
                }
            }*/
            break;

        default:
            free(p_new_vlan_rule);
            return GOS_FAIL;
    }

    if (PASS == pDsTci[0] && GOS_FAIL == omci_check_valid_rule(pConn, pTrafficRule, NULL))
    {
        free(p_new_vlan_rule);
        return GOS_OK;
    }

    processDsIgmpMcastTci(pMopTblEntry, pDsTci, pConn, index, p_new_vlan_rule);

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,
        "mcast gem port id=%u, uni mbpcd=%x, srvId=%u, vid=%u, fm=%u\n",
        p_new_vlan_rule->outgress, ((NULL != pConn->pUniPort) ? pConn->pUniPort->EntityID : 0x0),
        p_new_vlan_rule->servId, p_new_vlan_rule->vlanRule.filterRule.filterCTag.vid,
        p_new_vlan_rule->vlanRule.filterMode);

    if (GOS_FAIL == omci_AddTrafficRule(pConn, index, p_new_vlan_rule))
    {
        free(p_new_vlan_rule);
    }
    return GOS_OK;
}

//Note: when tpid is 88a8 and packet with ctag, Downstream_IGMP_and_multicast_TCI is set to add one tag, classfy rule only add stag
static GOS_ERROR_CODE
omci_AddMcastTrafficRule(MIB_TREE_CONN_T *pConn,int index, omci_vlan_rule_t *pTrafficRule)
{
    MIB_TABLE_GEMPORTCTP_T gemPortCtp, *pMibGemPortCtp = NULL;
    MIB_TABLE_MCASTOPERPROF_T mop, *pMop = NULL;
    MIB_TABLE_MULTIGEMIWTP_T multiGemIwtp;
    mopTableEntry_t *pMopTblEntry = NULL;
    struct aclHead *pAclHead;
    GOS_ERROR_CODE ret = GOS_OK;
    MIB_TABLE_MCASTSUBCONFINFO_T subConInfo, *pMibSubConInfo = NULL;
    MIB_TABLE_MACBRIPORTCFGDATA_T *pMibBridgePort = NULL, oldBridgePort;
    MIB_TABLE_ETHUNI_T mibPptpEthUNI;
    MIB_TABLE_VEIP_T mibVeip;

    memset(&gemPortCtp, 0, sizeof(MIB_TABLE_GEMPORTCTP_T));

    if(GOS_OK != MIB_GetFirst(MIB_TABLE_MULTIGEMIWTP_INDEX, &multiGemIwtp, sizeof(MIB_TABLE_MULTIGEMIWTP_T)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "can't find multicast gem iwtp");
        goto ok_free;
    }
    gemPortCtp.EntityID = multiGemIwtp.GemCtpPtr;
    if(FALSE == mib_FindEntry(MIB_TABLE_GEMPORTCTP_INDEX, &gemPortCtp, &pMibGemPortCtp))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "can't find GemPortCtp 0x%x", gemPortCtp.EntityID);
        goto ok_free;
    }
    //check ingress is multicast gem port Id
    if(pTrafficRule->outgress == pMibGemPortCtp->PortID)
    {
        memset(&oldBridgePort, 0, sizeof(MIB_TABLE_MACBRIPORTCFGDATA_T));
        // TBD;
        if (!pConn->pUniPort)
            goto ok_free;
        oldBridgePort.EntityID = pConn->pUniPort->EntityID;
        if (FALSE == mib_FindEntry(MIB_TABLE_MACBRIPORTCFGDATA_INDEX, &oldBridgePort, &pMibBridgePort))
            goto ok_free;

        memset(&subConInfo, 0, sizeof(MIB_TABLE_MCASTSUBCONFINFO_T));
        subConInfo.EntityId = pConn->pUniPort->EntityID;
        mibPptpEthUNI.EntityID = pMibBridgePort->TPPointer;
        mibVeip.EntityId = pMibBridgePort->TPPointer;

        if(FALSE == mib_FindEntry(MIB_TABLE_MCASTSUBCONFINFO_INDEX, &subConInfo, &pMibSubConInfo) ||
            ((GOS_OK != MIB_Get(MIB_TABLE_ETHUNI_INDEX, &mibPptpEthUNI, sizeof(MIB_TABLE_ETHUNI_T))) &&
            (GOS_OK != MIB_Get(MIB_TABLE_VEIP_INDEX, &mibVeip, sizeof(MIB_TABLE_VEIP_T)))))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "can't find UNI releated by entityID=%u", ((NULL != pConn->pUniPort) ? pConn->pUniPort->EntityID : 0x0));
            goto ok_free;
        }

        LIST_FOREACH(pMopTblEntry, &pMibSubConInfo->MOPhead, entries)
        {
            memset(&mop, 0, sizeof(MIB_TABLE_MCASTOPERPROF_T));
            mop.EntityId = pMopTblEntry->tableEntry.mcastOperProfPtr;
            if(FALSE == mib_FindEntry(MIB_TABLE_MCASTOPERPROF_INDEX, &mop, &pMop))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG, "can't find MOP 0x%x", mop.EntityId);
                continue;
            }
            feature_api(FEATURE_API_MC_00000001, pMop->DownstreamIgmpMulticastTci);
            if(pMop->curDaclCnt == 0 && pMop->curSaclCnt == 0)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG, "There are no any acl rule");
                ret = omci_AddMcastTrafficRuleByNoAcl(pMopTblEntry, pConn, index, pTrafficRule, pMop->DownstreamIgmpMulticastTci);
                if(GOS_OK!= ret)
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s()%d: ret:%d ", __FUNCTION__, __LINE__,ret);
            }
            else
            {
                //traffic rule for static multicast streaming according to static acl rule entries ANI vid.
                if(pMop->curSaclCnt)
                {
                    pAclHead = &pMop->SACLhead;
                    ret = omci_AddMcastTrafficRuleByAcl(pMopTblEntry, pAclHead, pConn, index, pTrafficRule, pMop->DownstreamIgmpMulticastTci);
                }
                //traffic rule for multicast streaming according to dynamic acl rule entries ANI vid.
                if(pMop->curDaclCnt)
                {
                    pAclHead = &pMop->DACLhead;
                    ret = omci_AddMcastTrafficRuleByAcl(pMopTblEntry, pAclHead, pConn, index, pTrafficRule, pMop->DownstreamIgmpMulticastTci);
                }
            }
        }
    }
ok_free:
    if(pTrafficRule)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s()%d: free vlan rule ", __FUNCTION__, __LINE__);
        free(pTrafficRule);
    }
    return GOS_OK;
}

static BOOL omci_is_mapper_to_one_gem(MIB_TREE_CONN_T *pConn)
{
    if (!pConn)
        return FALSE;

    if (!pConn->p8021Map)
        return FALSE;

    if (pConn->p8021Map->IwTpPtrPbit0 == pConn->p8021Map->IwTpPtrPbit1 && pConn->p8021Map->IwTpPtrPbit0 == pConn->p8021Map->IwTpPtrPbit2 &&
        pConn->p8021Map->IwTpPtrPbit0 == pConn->p8021Map->IwTpPtrPbit3 && pConn->p8021Map->IwTpPtrPbit0 == pConn->p8021Map->IwTpPtrPbit4 &&
        pConn->p8021Map->IwTpPtrPbit0 == pConn->p8021Map->IwTpPtrPbit5 && pConn->p8021Map->IwTpPtrPbit0 == pConn->p8021Map->IwTpPtrPbit6 &&
        pConn->p8021Map->IwTpPtrPbit0 == pConn->p8021Map->IwTpPtrPbit7 && pConn->p8021Map->IwTpPtrPbit0 != 0xFFFF)
    {
        return TRUE;
    }
    return FALSE;
}

static GOS_ERROR_CODE omci_ApplyTrafficRule(MIB_TREE_CONN_T *pConn,int index)
{
    int ret;
    omci_vlan_rule_t *pEntry;

    omci_set_tpid();

    /*check filter rule is the same*/
    LIST_FOREACH(pEntry,&pConn->ruleHead[index],entries)
    {
        // if flow based, without checking priority data bit.
        if (OMCI_TRAF_MODE_FLOW_BASE == pConn->traffMode && VLAN_OPER_MODE_EXTVLAN != pEntry->vlanRule.filterMode
            && VLAN_OPER_MODE_VLANTAG_OPER != pEntry->vlanRule.filterMode && omci_is_mapper_to_one_gem(pConn))
        {
            if(pEntry->vlanRule.filterRule.filterCtagMode & VLAN_FILTER_PRI)
                pEntry->vlanRule.filterRule.filterCtagMode = VLAN_FILTER_VID;
            if(pEntry->vlanRule.filterRule.filterStagMode & VLAN_FILTER_PRI)
                pEntry->vlanRule.filterRule.filterStagMode = VLAN_FILTER_VID;
            // modify priority to prevent legal value misconfiguration
            pEntry->vlanRule.outStyle.outVlan.pri = OMCI_PRI_FILTER_IGNORE;
        }
        if((ret = omci_wrapper_activeBdgConn(pEntry))!=GOS_OK)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: Apply Traffic Rule failed, ret=%d!",__FUNCTION__,ret);
            return GOS_OK;
        }
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_GetExtVlanTpidAct(unsigned int intputTpid, unsigned int fOutTpidID, unsigned int fInTpidID,
    unsigned int outputTpid, unsigned int tTpidID, OMCI_VLAN_OUT_ts *pOutVlan)
{
    /*handle tpid, NOTE: Not support DEI */
    switch (tTpidID)
    {
        case EVTOCD_TBL_TREATMENT_TPID_COPY_FROM_INNER:
            //recv Inner tag TPID
            switch (fInTpidID)
            {
                case EVTOCD_TBL_FILTER_TPID_8100:
                    pOutVlan->tpid = TPID_8100;
                    break;
                case EVTOCD_TBL_FILTER_TPID_INPUT_TPID:
                    //don't care DEI
                    pOutVlan->tpid = OMCI_GET_TPID_VALUE(intputTpid);
                    break;
                case EVTOCD_TBL_FILTER_TPID_INPUT_TPID_DEI_0:
                    //DEI = 0;
                    pOutVlan->tpid = OMCI_GET_TPID_VALUE(intputTpid);
                    break;
                case EVTOCD_TBL_FILTER_TPID_INPUT_TPID_DEI_1:
                    //DEI = 1;
                    pOutVlan->tpid = OMCI_GET_TPID_VALUE(intputTpid);
                    break;
            }
            break;
        case EVTOCD_TBL_TREATMENT_TPID_COPY_FROM_OUTER:
            //recv Outer tag TPID
            switch (fOutTpidID)
            {
                case EVTOCD_TBL_FILTER_TPID_8100:
                    pOutVlan->tpid = TPID_8100;
                    break;
                case EVTOCD_TBL_FILTER_TPID_INPUT_TPID:
                    //don't care DEI
                    pOutVlan->tpid = OMCI_GET_TPID_VALUE(intputTpid);
                    break;
                case EVTOCD_TBL_FILTER_TPID_INPUT_TPID_DEI_0:
                    //DEI = 0;
                    pOutVlan->tpid = OMCI_GET_TPID_VALUE(intputTpid);
                    break;
                case EVTOCD_TBL_FILTER_TPID_INPUT_TPID_DEI_1:
                    //DEI = 1;
                    pOutVlan->tpid = OMCI_GET_TPID_VALUE(intputTpid);
                    break;
            }
            break;
        case EVTOCD_TBL_TREATMENT_TPID_OUTPUT_DEI_CP_INNER:
            //DE=recv inner tag DE bit
            pOutVlan->tpid = OMCI_GET_TPID_VALUE(outputTpid);
            break;
        case EVTOCD_TBL_TREATMENT_TPID_OUTPUT_DEI_CP_OUTER:
            //DE=recv outer tag DE bit
            pOutVlan->tpid = OMCI_GET_TPID_VALUE(outputTpid);
            break;
        case EVTOCD_TBL_TREATMENT_TPID_8100:
            pOutVlan->tpid = TPID_8100;
            break;
        case EVTOCD_TBL_TREATMENT_TPID_OUTPUT_TPID_DEI_0:
            //DE=0
            pOutVlan->tpid = OMCI_GET_TPID_VALUE(outputTpid);
            break;
        case EVTOCD_TBL_TREATMENT_TPID_OUTPUT_TPID_DEI_1:
            //DE=1
            pOutVlan->tpid = OMCI_GET_TPID_VALUE(outputTpid);
            break;
    }
    return GOS_OK;
}

static UINT8 omci_GetVlanFilterMode(UINT8 fwdOp)
{
    UINT8 mode;

    switch(fwdOp){
    case VTFD_FWD_OP_TAG_A_UNTAG_A:
    case VTFD_FWD_OP_VID_G_UNTAG_A:
    case VTFD_FWD_OP_PRI_G_UNTAG_A:
    case VTFD_FWD_OP_TCI_G_UNTAG_A:
    {

        OMCI_LOG(OMCI_LOG_LEVEL_INFO,"%s: forward all",__FUNCTION__);
        mode = OMCI_VLANFILTER_MODE_FORWARDALL;
    }
    break;
    case VTFD_FWD_OP_TAG_C_UNTAG_A:
    {
        OMCI_LOG(OMCI_LOG_LEVEL_INFO,"%s: filter > drop tag, forward untag",__FUNCTION__);
        mode = OMCI_VLANFILTER_MODE_DROPTAG_FOWARDUNTAG;
    }
    break;
    case VTFD_FWD_OP_TAG_A_UNTAG_C:
    case VTFD_FWD_OP_VID_G_UNTAG_C:
    case VTFD_FWD_OP_PRI_G_UNTAG_C:
    case VTFD_FWD_OP_TCI_G_UNTAG_C:
    {
        OMCI_LOG(OMCI_LOG_LEVEL_INFO,"%s: filter > forward tag, drop untag",__FUNCTION__);
        mode = OMCI_VLANFILTER_MODE_FORWARDTAG_DROPUNTAG;

    }
    break;
    case VTFD_FWD_OP_VID_F_UNTAG_A:
    case VTFD_FWD_OP_VID_H_UNTAG_A:
    case VTFD_FWD_OP_VID_K_UNTAG_A:
    {
        OMCI_LOG(OMCI_LOG_LEVEL_INFO,"%s: filter > forward list VID,others drop, forward untag",__FUNCTION__);
        mode = OMCI_VLANFILTER_MODE_FORWARDLIST_VID_FORWARDUNTAG;
    }
    break;
    case VTFD_FWD_OP_PRI_F_UNTAG_A:
    case VTFD_FWD_OP_PRI_H_UNTAG_A:
    case VTFD_FWD_OP_PRI_K_UNTAG_A:
    {
        OMCI_LOG(OMCI_LOG_LEVEL_INFO,"%s: filter > forward list PRI,others drop, forward untag",__FUNCTION__);
        mode = OMCI_VLANFILTER_MODE_FORWARDLIST_PRI_FORWARDUNTAG;
    }
    break;
    case VTFD_FWD_OP_TCI_F_UNTAG_A:
    case VTFD_FWD_OP_TCI_H_UNTAG_A:
    case VTFD_FWD_OP_TCI_K_UNTAG_A:
    {
        OMCI_LOG(OMCI_LOG_LEVEL_INFO,"%s: filter > forward list TCI,others drop, forward untag",__FUNCTION__);
        mode = OMCI_VLANFILTER_MODE_FORWARDLIST_TCI_FORWARDUNTAG;
    }
    break;
    case VTFD_FWD_OP_VID_F_UNTAG_C:
    case VTFD_FWD_OP_VID_H_UNTAG_C:
    case VTFD_FWD_OP_VID_K_UNTAG_C:
    {
        OMCI_LOG(OMCI_LOG_LEVEL_INFO,"%s: filter > forward list VID,others drop, drop untag",__FUNCTION__);
        mode = OMCI_VLANFILTER_MODE_FORWARDLIST_VID_DROPUNTAG;
    }
    break;
    case VTFD_FWD_OP_PRI_F_UNTAG_C:
    case VTFD_FWD_OP_PRI_H_UNTAG_C:
    case VTFD_FWD_OP_PRI_K_UNTAG_C:
    {
        OMCI_LOG(OMCI_LOG_LEVEL_INFO,"%s: filter > forward list PRI,others drop, drop untag",__FUNCTION__);
        mode = OMCI_VLANFILTER_MODE_FORWARDLIST_PRI_DROPUNTAG;
    }
    break;
    case VTFD_FWD_OP_TCI_F_UNTAG_C:
    case VTFD_FWD_OP_TCI_H_UNTAG_C:
    case VTFD_FWD_OP_TCI_K_UNTAG_C:
    {
        OMCI_LOG(OMCI_LOG_LEVEL_INFO,"%s: filter > forward list TCI,others drop, drop untag",__FUNCTION__);
        mode = OMCI_VLANFILTER_MODE_FORWARDLIST_TCI_DROPUNTAG;
    }
    break;
    default:
        mode = OMCI_VLANFILTER_MODE_NOTSUPPORT;
    break;
    }

    return mode;
}

static UINT8 omci_GetVlanFilterModeFromAniUni(MIB_TREE_CONN_T *pConn)
{
    OMCI_VLANFILTER_MODE_T mode = OMCI_VLANFILTER_MODE_NOTSUPPORT, aniMode = OMCI_VLANFILTER_MODE_NOTSUPPORT, uniMode = OMCI_VLANFILTER_MODE_NOTSUPPORT;
    MIB_TABLE_VLANTAGFILTERDATA_T *pAniVlanFilter, *pUniVlanFilter;

    pAniVlanFilter = pConn->pAniVlanTagFilter;
    pUniVlanFilter = pConn->pUniVlanTagFilter;

    if(pAniVlanFilter)
        aniMode = omci_GetVlanFilterMode(pAniVlanFilter->FwdOp);
    else
        aniMode = OMCI_VLANFILTER_MODE_FORWARDALL;
    if(pUniVlanFilter)
        uniMode = omci_GetVlanFilterMode(pUniVlanFilter->FwdOp);
    else
        uniMode = OMCI_VLANFILTER_MODE_FORWARDALL;

    /*TBD, Mix the UNI and ANI VLAN tagging filter mode, use ANI first temporarily */
    if(aniMode == uniMode)
        mode = aniMode;
    else if (OMCI_VLANFILTER_MODE_FORWARDLIST_VID_DROPUNTAG == uniMode && OMCI_VLANFILTER_MODE_FORWARDLIST_VID_FORWARDUNTAG == aniMode)
        mode = uniMode;
    else if ((OMCI_VLANFILTER_MODE_FORWARDALL < aniMode) && (aniMode < OMCI_VLANFILTER_MODE_NOTSUPPORT))
        mode = aniMode;
    else if ((OMCI_VLANFILTER_MODE_FORWARDALL < uniMode) && (uniMode < OMCI_VLANFILTER_MODE_NOTSUPPORT))
        mode = uniMode;
    else if(aniMode == uniMode)
        mode = aniMode;

    return mode;
}

static GOS_ERROR_CODE omci_CreateTraffRule(
    MIB_TREE_CONN_T *pConn, int index, omci_vlan_rule_t **pRule, int ingress)
{
    *pRule = (omci_vlan_rule_t*)malloc(sizeof(omci_vlan_rule_t));
    (*pRule)->dir   = pConn->pGemPortCtp[index]->Direction;
    (*pRule)->ingress = ingress;
    (*pRule)->outgress = pConn->pGemPortCtp[index]->PortID;
    memset(&(*pRule)->vlanRule,0,sizeof(OMCI_VLAN_OPER_ts));

    (*pRule)->vlanRule.filterRule.filterStagMode = VLAN_FILTER_NO_CARE_TAG;
    (*pRule)->vlanRule.filterRule.filterCtagMode = VLAN_FILTER_NO_CARE_TAG;

    (*pRule)->vlanRule.sTagAct.vlanAct = VLAN_ACT_TRANSPARENT;
    (*pRule)->vlanRule.cTagAct.vlanAct = VLAN_ACT_TRANSPARENT;

    // initial the outstyle vid/pri to prevent misconfiguration
    (*pRule)->vlanRule.outStyle.outVlan.pri = OMCI_PRI_FILTER_IGNORE;
    (*pRule)->vlanRule.outStyle.outVlan.vid = OMCI_VID_FILTER_IGNORE;
    (*pRule)->vlanRule.outStyle.isMCRule = FALSE;

    return GOS_OK;
}

static void
omci_MergeAniVlanTagOperRule(
MIB_TREE_CONN_T *pConn, OMCI_VLAN_OPER_ts *pVlanRule, OMCI_VLAN_OUT_ts *pOutVlan)
{

    MIB_TABLE_VLANTAGOPCFGDATA_T *pVlanTagOpCfg;
    pVlanTagOpCfg = (MIB_TABLE_VLANTAGOPCFGDATA_T *)pConn->pAniVlanTagOpCfg;

    switch(pVlanTagOpCfg->UsTagOpMode)
    {
        case VTOCD_US_VLAN_TAG_OP_MODE_AS_IS:
            break;
        case VTOCD_US_VLAN_TAG_OP_MODE_MODIFY:
            pVlanRule->cTagAct.vlanAct = VLAN_ACT_MODIFY;
            pVlanRule->cTagAct.vidAct  = VID_ACT_ASSIGN;
            pVlanRule->cTagAct.priAct  = PRI_ACT_ASSIGN;
            pVlanRule->cTagAct.assignVlan.pri = (pVlanTagOpCfg->UsTagTci >> 13);
            pVlanRule->cTagAct.assignVlan.vid = (0xfff & pVlanTagOpCfg->UsTagTci);
            pVlanRule->cTagAct.assignVlan.tpid = UINT_MAX;
            memcpy(&pOutVlan->outVlan, &pVlanRule->cTagAct.assignVlan, sizeof(OMCI_VLAN_ts));
            break;
        case VTOCD_US_VLAN_TAG_OP_MODE_INSERT:
            //TBD change svlan tpid = 8100, do s-action
            break;
        default:
            break;
    }
    pOutVlan->dsTagOperMode = (VTOCD_DS_VLAN_TAG_OP_MODE_REMOVE == pVlanTagOpCfg->DsTagOpMode ? VLAN_ACT_REMOVE :
        (pVlanTagOpCfg->UsTagOpMode == pVlanTagOpCfg->DsTagOpMode ?
           ((FAL_OK != feature_api_is_registered(FEATURE_API_BDP_00000008_GEN_RULE)) ?
           VLAN_ACT_TRANSPARENT : VLAN_ACT_REMOVE) : VLAN_ACT_REMOVE));
}

static int omci_GetIngressByUniMbpcd(MIB_TREE_CONN_T *pConn)
{
    MIB_TABLE_MACBRIPORTCFGDATA_T *pUniPort = (MIB_TABLE_MACBRIPORTCFGDATA_T*)pConn->pUniPort;

    if (!pUniPort)
        return -1;

    if (pUniPort->TPType == MBPCD_TP_TYPE_PPTP_ETH_UNI)
    {
        if (!(pConn->pEthUni))
            return -1;

        return pConn->pEthUni->EntityID;
    }
    else if (pUniPort->TPType == MBPCD_TP_TYPE_VEIP)
    {
        if (!(pConn->pVeip))
            return -1;

        return pConn->pVeip->EntityId;
    }
    else if (pUniPort->TPType == MBPCD_TP_TYPE_IP_HOST_IPV6_HOST)
    {
        if (!(pConn->pIpHost))
            return -1;

        return pConn->pIpHost->EntityID;
    }
    return -1;
}

static GOS_ERROR_CODE omci_GenForwardUnTagOnlyPerIngress(
    MIB_TREE_CONN_T *pConn, int index, BOOL flag, int ingress)
{
    GOS_ERROR_CODE ret = GOS_OK;
    omci_vlan_rule_t *pRule;
    OMCI_VLAN_OPER_ts *pVlanRule;
    OMCI_VLAN_OUT_ts *pOutVlan;

    omci_CreateTraffRule(pConn,index, &pRule, ingress);
    pVlanRule = &pRule->vlanRule;
    pOutVlan = &pVlanRule->outStyle;
    pVlanRule->filterMode = VLAN_OPER_MODE_FORWARD_UNTAG;

    if (pConn->pAniVlanTagOpCfg && !pConn->pUniVlanTagOpCfg)
    {
        omci_MergeAniVlanTagOperRule(pConn, pVlanRule, pOutVlan);
        pVlanRule->filterMode = VLAN_OPER_MODE_VLANTAG_OPER;
    }
    else if (!pConn->pAniVlanTagOpCfg && pConn->pUniVlanTagOpCfg)
    {
        pOutVlan->dsTagOperMode = (VTOCD_DS_VLAN_TAG_OP_MODE_REMOVE ==
            ((MIB_TABLE_VLANTAGOPCFGDATA_T *)(pConn->pUniVlanTagOpCfg))->DsTagOpMode ? VLAN_ACT_REMOVE :
                                   (((MIB_TABLE_VLANTAGOPCFGDATA_T *)(pConn->pUniVlanTagOpCfg))->UsTagOpMode ==
                                    ((MIB_TABLE_VLANTAGOPCFGDATA_T *)(pConn->pUniVlanTagOpCfg))->DsTagOpMode ?
                                    ((FAL_OK != feature_api_is_registered(FEATURE_API_BDP_00000008_GEN_RULE)) ?
                                    VLAN_ACT_TRANSPARENT : VLAN_ACT_REMOVE) : VLAN_ACT_REMOVE));
        pVlanRule->filterMode = VLAN_OPER_MODE_VLANTAG_OPER;
    }

    if(GOS_OK != omci_UpdateTrafficRuleByManual(pConn, index, pRule))
        goto fail_free;

    if (GOS_FAIL == feature_api(FEATURE_API_BDP_00000040, pRule))
        goto fail_free;

    if (NULL == pConn->pUniExtVlanCfg || TRUE == flag)
    {
        ret = (((pRule->dir == PON_GEMPORT_DIRECTION_DS) &&
                (NULL != pConn->pMcastGemIwTp) &&
                (GOS_OK != omci_McTransparentFwd(pConn, pRule))) ?
                omci_AddMcastTrafficRule(pConn, index, pRule) : omci_AddTrafficRule(pConn, index, pRule));
        if (GOS_OK == ret)
            return ret;
    }

fail_free:
    if(pRule)
        free(pRule);
    return ret;
}

static GOS_ERROR_CODE omci_GenForwardUnTagOnly(MIB_TREE_CONN_T *pConn, int index, BOOL flag)
{
    GOS_ERROR_CODE ret = GOS_OK;
    int ingress;

    ingress = omci_GetIngressByUniMbpcd(pConn);

    ret = omci_GenForwardUnTagOnlyPerIngress(pConn, index, flag, ingress);

    return ret;
}

static GOS_ERROR_CODE omci_GenForwardSingleTagOnlyPerIngress(
    MIB_TREE_CONN_T *pConn, int index, BOOL flag, int ingress)
{
    GOS_ERROR_CODE ret  = GOS_OK;;
    omci_vlan_rule_t *pRule;
    OMCI_VLAN_OPER_ts *pVlanRule;
    OMCI_VLAN_OUT_ts *pOutVlan;

    omci_CreateTraffRule(pConn, index, &pRule, ingress);
    pVlanRule = &pRule->vlanRule;
    pOutVlan = &pVlanRule->outStyle;
    pVlanRule->filterMode = VLAN_OPER_MODE_FORWARD_SINGLETAG;

    if (pConn->pAniVlanTagOpCfg && !pConn->pUniVlanTagOpCfg)
    {
        omci_MergeAniVlanTagOperRule(pConn, pVlanRule, pOutVlan);
        pVlanRule->filterMode = VLAN_OPER_MODE_VLANTAG_OPER;
    }
    else if (!pConn->pAniVlanTagOpCfg && pConn->pUniVlanTagOpCfg)
    {
        pOutVlan->dsTagOperMode = (VTOCD_DS_VLAN_TAG_OP_MODE_REMOVE ==
            ((MIB_TABLE_VLANTAGOPCFGDATA_T *)(pConn->pUniVlanTagOpCfg))->DsTagOpMode ? VLAN_ACT_REMOVE :
            (((MIB_TABLE_VLANTAGOPCFGDATA_T *)(pConn->pUniVlanTagOpCfg))->DsTagOpMode ==
                ((MIB_TABLE_VLANTAGOPCFGDATA_T *)(pConn->pUniVlanTagOpCfg))->UsTagOpMode ?
                ((FAL_OK != feature_api_is_registered(FEATURE_API_BDP_00000008_GEN_RULE)) ?
                VLAN_ACT_TRANSPARENT : VLAN_ACT_REMOVE) : VLAN_ACT_REMOVE));
        pVlanRule->filterMode = VLAN_OPER_MODE_VLANTAG_OPER;
    }

    if (GOS_OK != omci_UpdateTrafficRuleByManual(pConn, index, pRule))
        goto fail_free;

    if (GOS_FAIL == feature_api(FEATURE_API_BDP_00000040, pRule))
        goto fail_free;

    if (NULL == pConn->pUniExtVlanCfg || TRUE == flag)
    {
        ret = (((pRule->dir == PON_GEMPORT_DIRECTION_DS) &&
                (NULL != pConn->pMcastGemIwTp) &&
                (GOS_OK != omci_McTransparentFwd(pConn, pRule))) ?
                omci_AddMcastTrafficRule(pConn, index, pRule) : omci_AddTrafficRule(pConn, index, pRule));
        if (GOS_OK == ret)
            return ret;
    }

fail_free:
    if(pRule)
        free(pRule);
    return ret;
}

static GOS_ERROR_CODE omci_GenForwardSingleTagOnly(MIB_TREE_CONN_T *pConn, int index, BOOL flag)
{
    GOS_ERROR_CODE ret = GOS_OK;;
    int ingress;

    ingress = omci_GetIngressByUniMbpcd(pConn);

    ret = omci_GenForwardSingleTagOnlyPerIngress(pConn, index, flag, ingress);

    return ret;
}

static BOOL omci_check_ani_uni_tci(
    UINT8 uFwdOp, UINT8 aFwdOp, UINT16 uTci, UINT16 aTci, UINT16 *pMtci)
{
    BOOL                    ret = FALSE;
    OMCI_VLANFILTER_MODE_T  uMode = OMCI_VLANFILTER_MODE_NOTSUPPORT;
    OMCI_VLANFILTER_MODE_T  aMode = OMCI_VLANFILTER_MODE_NOTSUPPORT;

    aMode = omci_GetVlanFilterMode(aFwdOp);
    uMode = omci_GetVlanFilterMode(uFwdOp);

    switch (aMode)
    {
        case OMCI_VLANFILTER_MODE_FORWARDLIST_VID_FORWARDUNTAG:
        case OMCI_VLANFILTER_MODE_FORWARDLIST_VID_DROPUNTAG:
            switch (uMode)
            {
                case OMCI_VLANFILTER_MODE_FORWARDLIST_VID_FORWARDUNTAG:
                case OMCI_VLANFILTER_MODE_FORWARDLIST_VID_DROPUNTAG:
                    if ((aTci & 0xFFF) == (uTci & 0xFFF))
                    {
                        ret = TRUE;
                        *pMtci = aTci;
                    }
                    break;
                case OMCI_VLANFILTER_MODE_FORWARDLIST_PRI_FORWARDUNTAG:
                case OMCI_VLANFILTER_MODE_FORWARDLIST_PRI_DROPUNTAG:
                    ret = TRUE;
                    *pMtci = ((aTci & 0x1FFF) | (uTci & 0xE000));
                    break;
                case OMCI_VLANFILTER_MODE_FORWARDLIST_TCI_FORWARDUNTAG:
                case OMCI_VLANFILTER_MODE_FORWARDLIST_TCI_DROPUNTAG:
                    if ((aTci & 0xFFF) == (uTci & 0xFFF))
                    {
                        ret = TRUE;
                        *pMtci = ((aTci & 0xFFF) | (uTci & 0xF000));
                    }
                    break;
                default:
                    ret = FALSE; //TBD
            }
            break;
        case OMCI_VLANFILTER_MODE_FORWARDLIST_PRI_FORWARDUNTAG:
        case OMCI_VLANFILTER_MODE_FORWARDLIST_PRI_DROPUNTAG:
            switch (uMode)
            {
                case OMCI_VLANFILTER_MODE_FORWARDLIST_VID_FORWARDUNTAG:
                case OMCI_VLANFILTER_MODE_FORWARDLIST_VID_DROPUNTAG:
                    ret = TRUE;
                    *pMtci = ((uTci & 0x1FFF) | (aTci & 0xE000));
                    break;
                case OMCI_VLANFILTER_MODE_FORWARDLIST_PRI_FORWARDUNTAG:
                case OMCI_VLANFILTER_MODE_FORWARDLIST_PRI_DROPUNTAG:
                    if ((aTci & 0xE000) == (uTci & 0xE000))
                    {
                        ret = TRUE;
                        *pMtci = aTci;
                    }
                    break;
                case OMCI_VLANFILTER_MODE_FORWARDLIST_TCI_FORWARDUNTAG:
                case OMCI_VLANFILTER_MODE_FORWARDLIST_TCI_DROPUNTAG:
                    if ((aTci & 0xE000) == (uTci & 0xE000))
                    {
                        ret = TRUE;
                        *pMtci = ((uTci & 0x1FFF) | (aTci & 0xE000));
                    }
                    break;
                default:
                    ret = FALSE; //TBD
            }
            break;
        case OMCI_VLANFILTER_MODE_FORWARDLIST_TCI_FORWARDUNTAG:
        case OMCI_VLANFILTER_MODE_FORWARDLIST_TCI_DROPUNTAG:
            switch (uMode)
            {
                case OMCI_VLANFILTER_MODE_FORWARDLIST_VID_FORWARDUNTAG:
                case OMCI_VLANFILTER_MODE_FORWARDLIST_VID_DROPUNTAG:
                    if ((aTci & 0xFFF) == (uTci & 0xFFF))
                    {
                        ret = TRUE;
                        *pMtci = aTci;
                    }
                    break;
                case OMCI_VLANFILTER_MODE_FORWARDLIST_PRI_FORWARDUNTAG:
                case OMCI_VLANFILTER_MODE_FORWARDLIST_PRI_DROPUNTAG:
                    if ((aTci & 0xE000) == (uTci & 0xE000))
                    {
                        ret = TRUE;
                        *pMtci = aTci;
                    }
                    break;
                case OMCI_VLANFILTER_MODE_FORWARDLIST_TCI_FORWARDUNTAG:
                case OMCI_VLANFILTER_MODE_FORWARDLIST_TCI_DROPUNTAG:
                    if (aTci == uTci)
                    {
                        ret = TRUE;
                        *pMtci = aTci;
                    }
                    break;
                default:
                    ret = FALSE; //TBD
            }
            break;
        default:
            ret = FALSE; //TBD
    }

    return ret;
}

static GOS_ERROR_CODE omci_GenVlanFilterVlanTbl(MIB_TREE_CONN_T *pConn, UINT16 *tciTbl, int *tblSize)
{
    int i, j;
    int arraySizeAni = 0, arraySizeUni = 0;
    UINT16 tciAni, tciUni, mergeTci;
    MIB_TABLE_VLANTAGFILTERDATA_T *pAniVlanFilter = pConn->pAniVlanTagFilter;
    MIB_TABLE_VLANTAGFILTERDATA_T *pUniVlanFilter = pConn->pUniVlanTagFilter;

    if((tciTbl == NULL) || (tblSize == NULL))
        return GOS_FAIL;

    if(pAniVlanFilter)
        arraySizeAni = pAniVlanFilter->NumOfEntries*2;
    if(pUniVlanFilter && VTFD_FWD_OP_TAG_A_UNTAG_A != pUniVlanFilter->FwdOp)
        arraySizeUni = pUniVlanFilter->NumOfEntries*2;

    *tblSize = 0;

    /*Compare Ani and Uni vlanFilter list*/
    if((arraySizeAni != 0)&& (arraySizeUni != 0))
    {

        for(i = 0; i < arraySizeAni; i+=2)
        {
            tciAni = pAniVlanFilter->FilterTbl[i] << 8 | pAniVlanFilter->FilterTbl[i+1];
            for(j = 0; j < arraySizeUni; j+=2)
            {
                tciUni = pUniVlanFilter->FilterTbl[j] << 8 | pUniVlanFilter->FilterTbl[j+1];
                // check fwd operation, ani_tci and uni_tci
                if (omci_check_ani_uni_tci(
                    pUniVlanFilter->FwdOp, pAniVlanFilter->FwdOp,
                    tciUni, tciAni, &mergeTci))
                {
                    tciTbl[(*tblSize)] = mergeTci;
                    (*tblSize)++;
                    break;
                }
            }
        }
    }
    else if(arraySizeAni != 0)
    {
        arraySizeAni = pAniVlanFilter->NumOfEntries*2;
        *tblSize = pAniVlanFilter->NumOfEntries;
        for(i = 0; i < arraySizeAni; i+=2)
        {
            tciTbl[i/2] = pAniVlanFilter->FilterTbl[i] << 8 | pAniVlanFilter->FilterTbl[i+1];
        }
    }
    else if(arraySizeUni != 0)
    {
        arraySizeUni = pUniVlanFilter->NumOfEntries*2;
        *tblSize = pUniVlanFilter->NumOfEntries;
        for(i = 0; i < arraySizeUni; i+=2)
        {
            tciTbl[i/2] = pUniVlanFilter->FilterTbl[i] << 8 | pUniVlanFilter->FilterTbl[i+1];
        }
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_GenVlanFilterRulePerIngress(
    MIB_TREE_CONN_T *pConn, unsigned int mode, int index, BOOL flag, UINT16 *tciTbl, int i, int ingress)
{
    GOS_ERROR_CODE ret = GOS_OK;
    omci_vlan_rule_t *pRule;
    OMCI_VLAN_OPER_ts *pVlanRule;
    OMCI_VLAN_OUT_ts *pOutVlan;

    omci_CreateTraffRule(pConn, index, &pRule, ingress);
    pVlanRule = &pRule->vlanRule;
    pOutVlan = &pVlanRule->outStyle;
    pVlanRule->filterMode = VLAN_OPER_MODE_FILTER_SINGLETAG;

    if(mode== OMCI_VLANFILTER_MODE_FORWARDLIST_VID_FORWARDUNTAG ||
    mode == OMCI_VLANFILTER_MODE_FORWARDLIST_TCI_FORWARDUNTAG ||
    mode == OMCI_VLANFILTER_MODE_FORWARDLIST_VID_DROPUNTAG ||
    mode == OMCI_VLANFILTER_MODE_FORWARDLIST_TCI_DROPUNTAG)
    {
        pVlanRule->filterRule.filterCtagMode &= ~(VLAN_FILTER_NO_CARE_TAG);
        pVlanRule->filterRule.filterCtagMode |= VLAN_FILTER_VID;
        pVlanRule->filterRule.filterCTag.vid = tciTbl[i] & 0xfff;
        if (pConn->pAniVlanTagOpCfg && !pConn->pUniVlanTagOpCfg)
        {
            omci_MergeAniVlanTagOperRule(pConn, pVlanRule, pOutVlan);
            pVlanRule->filterMode = VLAN_OPER_MODE_VLANTAG_OPER;
        }
        else if (!pConn->pAniVlanTagOpCfg && pConn->pUniVlanTagOpCfg)
        {
            pOutVlan->dsTagOperMode = (VTOCD_DS_VLAN_TAG_OP_MODE_REMOVE ==
                ((MIB_TABLE_VLANTAGOPCFGDATA_T *)(pConn->pUniVlanTagOpCfg))->DsTagOpMode ? VLAN_ACT_REMOVE :
                (((MIB_TABLE_VLANTAGOPCFGDATA_T *)(pConn->pUniVlanTagOpCfg))->DsTagOpMode ==
                    ((MIB_TABLE_VLANTAGOPCFGDATA_T *)(pConn->pUniVlanTagOpCfg))->UsTagOpMode ?
                    ((FAL_OK != feature_api_is_registered(FEATURE_API_BDP_00000008_GEN_RULE)) ?
                    VLAN_ACT_TRANSPARENT : VLAN_ACT_REMOVE) : VLAN_ACT_REMOVE));
            pVlanRule->filterMode = VLAN_OPER_MODE_VLANTAG_OPER;
        }
        else
            pOutVlan->outVlan.vid = tciTbl[i] & 0xfff;
    }

    if(mode== OMCI_VLANFILTER_MODE_FORWARDLIST_PRI_FORWARDUNTAG ||
    mode == OMCI_VLANFILTER_MODE_FORWARDLIST_TCI_FORWARDUNTAG ||
    mode == OMCI_VLANFILTER_MODE_FORWARDLIST_PRI_DROPUNTAG ||
    mode == OMCI_VLANFILTER_MODE_FORWARDLIST_TCI_DROPUNTAG)
    {
        pVlanRule->filterRule.filterCtagMode &= ~(VLAN_FILTER_NO_CARE_TAG);
        pVlanRule->filterRule.filterCtagMode |= VLAN_FILTER_PRI;
        pVlanRule->filterRule.filterCTag.pri = (tciTbl[i] >>13) & 0x7;
        if (pConn->pAniVlanTagOpCfg && !pConn->pUniVlanTagOpCfg)
        {
            omci_MergeAniVlanTagOperRule(pConn, pVlanRule, pOutVlan);
            pVlanRule->filterMode = VLAN_OPER_MODE_VLANTAG_OPER;
        }
        else if (!pConn->pAniVlanTagOpCfg && pConn->pUniVlanTagOpCfg)
        {
            pOutVlan->dsTagOperMode = (VTOCD_DS_VLAN_TAG_OP_MODE_REMOVE ==
                ((MIB_TABLE_VLANTAGOPCFGDATA_T *)(pConn->pUniVlanTagOpCfg))->DsTagOpMode ? VLAN_ACT_REMOVE :
                (((MIB_TABLE_VLANTAGOPCFGDATA_T *)(pConn->pUniVlanTagOpCfg))->DsTagOpMode ==
                    ((MIB_TABLE_VLANTAGOPCFGDATA_T *)(pConn->pUniVlanTagOpCfg))->UsTagOpMode ?
                    ((FAL_OK != feature_api_is_registered(FEATURE_API_BDP_00000008_GEN_RULE)) ?
                    VLAN_ACT_TRANSPARENT : VLAN_ACT_REMOVE) : VLAN_ACT_REMOVE));
            pVlanRule->filterMode = VLAN_OPER_MODE_VLANTAG_OPER;
        }
        else
            pOutVlan->outVlan.pri = (tciTbl[i] >>13) & 0x7;
    }else
    {
        if (OMCI_TRAF_MODE_8021P_BASE == pConn->traffMode)
        {
            pVlanRule->filterRule.filterCtagMode &= ~(VLAN_FILTER_NO_CARE_TAG);
            pVlanRule->filterRule.filterCtagMode |= VLAN_FILTER_PRI;
        }
        pVlanRule->filterRule.filterCTag.pri = index;
        pOutVlan->outVlan.pri = index;
    }
    pOutVlan->outTagNum = 1;

    if (GOS_OK != omci_UpdateTrafficRuleByManual(pConn, index, pRule))
        goto fail_free;

    if (GOS_FAIL == feature_api(FEATURE_API_BDP_00000040, pRule))
        goto fail_free;

    if (NULL == pConn->pUniExtVlanCfg || TRUE == flag)
    {
        ret = (((pRule->dir == PON_GEMPORT_DIRECTION_DS) &&
                (NULL != pConn->pMcastGemIwTp) &&
                (GOS_OK != omci_McTransparentFwd(pConn, pRule))) ?
                omci_AddMcastTrafficRule(pConn, index, pRule) : omci_AddTrafficRule(pConn, index, pRule));
        if (GOS_OK == ret)
            return ret;
    }

fail_free:
    if (pRule)
        free(pRule);
    return ret;
}

static GOS_ERROR_CODE omci_GenVlanFilterRule(MIB_TREE_CONN_T *pConn, unsigned int mode, int index, BOOL flag)
{
    int i, arraySize;
    GOS_ERROR_CODE ret = GOS_OK;
    UINT16 tciTbl[MIB_TABLE_VLANTAGFILTERDATA_FILTERTBL_LEN/2];
    int ingress;

    if (NULL != pConn->pUniExtVlanCfg && TRUE != flag)
    {
        return ret;
    }

    omci_GenVlanFilterVlanTbl(pConn, tciTbl, &arraySize);
    ingress = omci_GetIngressByUniMbpcd(pConn);

    for(i=0; i < arraySize; i++)
    {
        ret = omci_GenVlanFilterRulePerIngress(pConn, mode, index, flag, tciTbl, i, ingress);
    }
    return ret;
}


static GOS_ERROR_CODE omci_GetExtVlanPriAct(
        OMCI_VLAN_ACT_ts *pVlanAct, OMCI_VLAN_ts treatment, UINT8 tagNum)
{
    if (!pVlanAct)
        return GOS_ERR_PARAM;
    // tag number should be 0 ~ 2
    if (tagNum > 2)
        return GOS_ERR_PARAM;


    if (EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_INNER == treatment.pri)
    {
        if(0 == tagNum)
            return GOS_ERR_PARAM;
        if (2 == tagNum)
            pVlanAct->priAct = PRI_ACT_COPY_INNER;
        else
            // copy from 2nd tag is not possible for single-tag filter,
            // but we may tolerate this error and make it copy from 1st tag
            pVlanAct->priAct = PRI_ACT_COPY_OUTER;
    }
    else if (EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_OUTER == treatment.pri)
    {
        if(0 == tagNum)
            return GOS_ERR_PARAM;
        pVlanAct->priAct = PRI_ACT_COPY_OUTER;
    }
    else if (EVTOCD_TBL_TREATMENT_PRI_DERIVE_FROM_DSCP == treatment.pri)
    {
            pVlanAct->priAct = PRI_ACT_FROM_DSCP;
    }
    else
    {
        pVlanAct->priAct = PRI_ACT_ASSIGN;
        pVlanAct->assignVlan.pri= treatment.pri;
    }

    return GOS_OK;
}




static GOS_ERROR_CODE omci_GetExtVlanUntagAct(
unsigned int inputTpid,
unsigned int outputTpid,
OMCI_VLAN_ts fOut,
OMCI_VLAN_ts fIn,
OMCI_VLAN_ts tOut,
OMCI_VLAN_ts tIn,
OMCI_VLAN_ACT_ts *pSVlanAct,
OMCI_VLAN_ACT_ts *pCVlanAct,
OMCI_VLAN_OUT_ts *pOutVlan)
{
    /*default case, do nothing, F->F*/
    if(tOut.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG && tIn.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG)
    {
        pSVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
        pCVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
        pOutVlan->outTagNum= 0;
        pOutVlan->isDefaultRule = 1;

    }else if(tOut.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG)
    /*insert 1 full tag (X), F->X-F */
    {
        omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tIn.tpid, pOutVlan);
        if(TPID_88A8 == pOutVlan->tpid || TPID_9100 == pOutVlan->tpid)
        {
            pSVlanAct->vlanAct = VLAN_ACT_ADD;
            pSVlanAct->vidAct  = VID_ACT_ASSIGN;
            pSVlanAct->assignVlan = tIn;
            omci_GetExtVlanPriAct(pSVlanAct,tIn,0);
            pCVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
        }
        else if (TPID_8100 == pOutVlan->tpid)
        {
            pSVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
            pCVlanAct->vlanAct = VLAN_ACT_ADD;
            pCVlanAct->vidAct  = VID_ACT_ASSIGN;
            pCVlanAct->assignVlan = tIn;
            omci_GetExtVlanPriAct(pCVlanAct,tIn,0);
        }
        pOutVlan->outTagNum = 1;
        pOutVlan->outVlan = tIn;
    }else
    /*insert 2 full tag (X,Y), F->Y-X-F*/
    {
        omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tOut.tpid, pOutVlan);
        pSVlanAct->vlanAct = VLAN_ACT_ADD;
        pSVlanAct->vidAct  = VID_ACT_ASSIGN;
        pSVlanAct->assignVlan = tOut;
        omci_GetExtVlanPriAct(pSVlanAct,tOut,0);

        pCVlanAct->vlanAct = VLAN_ACT_ADD;
        pCVlanAct->vidAct  = VID_ACT_ASSIGN;
        pCVlanAct->assignVlan = tIn;
        omci_GetExtVlanPriAct(pCVlanAct,tIn,0);
        pOutVlan->outTagNum = 2;
        pOutVlan->outVlan  = tOut;
    }
    return GOS_OK;
}


static GOS_ERROR_CODE omci_GetExtVlanVidAct(
        OMCI_VLAN_ACT_ts *pVlanAct, OMCI_VLAN_ts treatment,
        OMCI_VLAN_ts filterIn,OMCI_VLAN_ts filterOut, UINT8 tagNum)
{
    if (!pVlanAct)
        return GOS_ERR_PARAM;
    // tag number should be 1 ~ 2
    if (tagNum < 1 || tagNum > 2)
        return GOS_ERR_PARAM;


    if (EVTOCD_TBL_TREATMENT_VID_COPY_FROM_INNER == treatment.vid)
    {
        if(EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER == filterIn.vid)
        {
            if (2 == tagNum)
                pVlanAct->vidAct = VID_ACT_COPY_INNER;
            else
                // copy from 2nd tag is not possible for single-tag filter,
                // but we may tolerate this error and make it copy from 1st tag
                pVlanAct->vidAct = VID_ACT_COPY_OUTER;
        }
        else
        {
            pVlanAct->vidAct = VID_ACT_ASSIGN;
            pVlanAct->assignVlan.vid = filterIn.vid;

        }

    }
    else if (EVTOCD_TBL_TREATMENT_VID_COPY_FROM_OUTER == treatment.vid)
    {
        if(EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER == filterIn.vid)
        {
            pVlanAct->vidAct = VID_ACT_COPY_OUTER;
        }
        else
        {
            pVlanAct->vidAct = VID_ACT_ASSIGN;
            pVlanAct->assignVlan.vid = filterOut.vid;

        }
    }
    else
    {
        pVlanAct->vidAct = VID_ACT_ASSIGN;
        pVlanAct->assignVlan.vid = treatment.vid;
    }

    return GOS_OK;
}



static GOS_ERROR_CODE omci_GetOutVlanVid(
        OMCI_VLAN_ts *pOutVlan, OMCI_VLAN_ts treatment, OMCI_VLAN_ts filterIn,OMCI_VLAN_ts filterOut)
{
    if (!pOutVlan)
        return GOS_ERR_PARAM;

    if (EVTOCD_TBL_TREATMENT_VID_COPY_FROM_INNER == treatment.vid)
    {
        pOutVlan->vid = filterIn.vid;
    }
    else if (EVTOCD_TBL_TREATMENT_VID_COPY_FROM_OUTER == treatment.vid)
    {
        pOutVlan->vid = filterOut.vid;
    }
    else
    {
        pOutVlan->vid = treatment.vid;
    }

    return GOS_OK;
}








static GOS_ERROR_CODE omci_GetExtVlanSingleTagAct(
unsigned int inputTpid,
unsigned int outputTpid,
OMCI_VLAN_ts fOut,
OMCI_VLAN_ts fIn,
OMCI_VLAN_ts tOut,
OMCI_VLAN_ts tIn,
OMCI_VLAN_ACT_ts *pSVlanAct,
OMCI_VLAN_ACT_ts *pCVlanAct,
unsigned int removeTag,
OMCI_VLAN_OUT_ts *pOutVlan)
{
    if(tOut.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG)
    {
        pSVlanAct->vlanAct = VLAN_ACT_NON;

        if(tIn.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG && 0 == removeTag)
        {
            pCVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
            pOutVlan->outTagNum = 1;
            pOutVlan->outVlan = fIn;
            if (fIn.tpid <= EVTOCD_TBL_FILTER_TPID_8100 ||
                (fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID && OMCI_GET_TPID_VALUE(inputTpid) == TPID_8100))
            {
                pOutVlan->tpid = TPID_8100;
            }
            else if (fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID && OMCI_GET_TPID_VALUE(inputTpid) == TPID_88A8)
            {
                pOutVlan->tpid = TPID_88A8;
            }
            else if (fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID && OMCI_GET_TPID_VALUE(inputTpid) == TPID_9100)
            {
                pOutVlan->tpid = TPID_9100;
            }

            if (fOut.pri == EVTOCD_TBL_FILTER_PRI_IGNORE_OTHER_FIELD &&
                    fOut.vid == EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER &&
                    fIn.vid == EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER &&
                    fIn.pri == EVTOCD_TBL_FILTER_PRI_DEFAULT_TAG_RULE)
                pOutVlan->isDefaultRule = 1;

        }else if(tIn.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG && removeTag)
        /*remove tag, CF->F */
        {
            pCVlanAct->vlanAct = VLAN_ACT_REMOVE;
            pOutVlan->outTagNum  = 0;

        }else if(tIn.pri == EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_INNER && removeTag)
        /*modify tag, keep original priority, C-F->X-F*/
        {
            omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tIn.tpid, pOutVlan);
            if(TPID_88A8 == pOutVlan->tpid || TPID_9100 == pOutVlan->tpid)
            {
                pSVlanAct->vlanAct = VLAN_ACT_ADD;
                pSVlanAct->priAct  = PRI_ACT_COPY_OUTER;
                pOutVlan->outVlan = tIn;
                omci_GetExtVlanVidAct(pSVlanAct, tIn, fIn, fOut, 1);

                if (fIn.tpid <= EVTOCD_TBL_FILTER_TPID_8100 ||
                        (fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID && OMCI_GET_TPID_VALUE(inputTpid) == TPID_8100))
                    pCVlanAct->vlanAct = VLAN_ACT_REMOVE;
            }
            else if(TPID_8100 == pOutVlan->tpid)
            {
                if(EVTOCD_TBL_TREATMENT_VID_COPY_FROM_INNER == tIn.vid)
                {
                    pCVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
                    pOutVlan->outVlan.vid = fIn.vid;
                }
                else
                {
                    pCVlanAct->vlanAct = VLAN_ACT_MODIFY;
                    pCVlanAct->priAct  = PRI_ACT_COPY_OUTER;
                    omci_GetExtVlanVidAct(pCVlanAct, tIn, fIn, fOut, 1);
                    pCVlanAct->assignVlan.tpid = tIn.tpid;
                    pOutVlan->outVlan = tIn;
                }
            }
            pOutVlan->outTagNum  = 1;
            omci_GetOutVlanVid(&pOutVlan->outVlan, tIn, fIn, fOut);
            pOutVlan->outVlan.pri = fIn.pri;
        }else if(tIn.pri == EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_INNER)
        /*insert 1 tag (X), copy priority, C-F->X-C-F*/
        {
            omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tIn.tpid, pOutVlan);
            pSVlanAct->vlanAct = VLAN_ACT_ADD;
            pSVlanAct->priAct  = PRI_ACT_COPY_OUTER;
            omci_GetExtVlanVidAct(pSVlanAct, tIn, fIn, fOut ,1);

            pOutVlan->outTagNum  = 2;
            pOutVlan->outVlan = tIn;
            pOutVlan->outVlan.pri = fIn.pri;
            omci_GetOutVlanVid(&pOutVlan->outVlan, tIn, fIn, fOut);


        }else if((tIn.pri == EVTOCD_TBL_TREATMENT_PRI_DERIVE_FROM_DSCP) && removeTag)
        /*modify tag, priority from DSCP, C-F->X-F*/
        {
            omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tIn.tpid, pOutVlan);
            if(TPID_88A8 == pOutVlan->tpid || TPID_9100 == pOutVlan->tpid)
            {
                pSVlanAct->vlanAct = VLAN_ACT_ADD;
                pSVlanAct->priAct  = PRI_ACT_FROM_DSCP;
                omci_GetExtVlanVidAct(pSVlanAct, tIn, fIn, fOut ,1);
                pSVlanAct->assignVlan.tpid = tIn.tpid;

                if (fIn.tpid <= EVTOCD_TBL_FILTER_TPID_8100 ||
                        (fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID && OMCI_GET_TPID_VALUE(inputTpid) == TPID_8100))
                    pCVlanAct->vlanAct = VLAN_ACT_REMOVE;
            }
            else if(TPID_8100 == pOutVlan->tpid)
            {

                pCVlanAct->vlanAct = VLAN_ACT_MODIFY;
                pCVlanAct->priAct  = PRI_ACT_FROM_DSCP;
                omci_GetExtVlanVidAct(pCVlanAct, tIn, fIn, fOut ,1);
                pCVlanAct->assignVlan.tpid = tIn.tpid;

            }
            pOutVlan->outTagNum  = 1;
            pOutVlan->outVlan = tIn;
            omci_GetOutVlanVid(&pOutVlan->outVlan, tIn, fIn, fOut);
            pOutVlan->outVlan.pri = fIn.pri;
        }
        else if((tIn.pri == EVTOCD_TBL_TREATMENT_PRI_DERIVE_FROM_DSCP) && (removeTag == 0))
        /*insert 1 tag (X), priority from DSCP, C-F->X-C-F*/
        {
            omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tIn.tpid, pOutVlan);
            pSVlanAct->vlanAct = VLAN_ACT_ADD;
            pSVlanAct->priAct  = PRI_ACT_FROM_DSCP;
            pSVlanAct->vidAct  = VID_ACT_ASSIGN;
            pSVlanAct->assignVlan = tIn;
            omci_GetExtVlanVidAct(pSVlanAct, tIn, fIn, fOut ,1);

            pOutVlan->outTagNum  = 2;
            pOutVlan->outVlan = tIn;
            pOutVlan->outVlan.pri = fIn.pri;
            omci_GetOutVlanVid(&pOutVlan->outVlan, tIn, fIn, fOut);

        }
        else if(removeTag)
        /*modify tag, C-F->X-F*/
        {
            omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tIn.tpid, pOutVlan);
            if(TPID_88A8 == pOutVlan->tpid || TPID_9100 == pOutVlan->tpid)
            {
                pSVlanAct->vlanAct = VLAN_ACT_MODIFY;
                pSVlanAct->priAct  = PRI_ACT_ASSIGN;
                pSVlanAct->assignVlan.pri = tIn.pri;
                pSVlanAct->assignVlan.tpid = tIn.tpid;
                omci_GetExtVlanVidAct(pSVlanAct, tIn, fIn, fOut ,1);

                if (fIn.tpid <= EVTOCD_TBL_FILTER_TPID_8100 ||
                        (fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID && OMCI_GET_TPID_VALUE(inputTpid) == TPID_8100))
                    pCVlanAct->vlanAct = VLAN_ACT_REMOVE;
            }
            else if(TPID_8100 == pOutVlan->tpid)
            {
                pCVlanAct->vlanAct = VLAN_ACT_MODIFY;
                pCVlanAct->priAct  = PRI_ACT_ASSIGN;
                pCVlanAct->assignVlan.pri = tIn.pri;
                pCVlanAct->assignVlan.tpid = tIn.tpid;
                omci_GetExtVlanVidAct(pCVlanAct, tIn, fIn, fOut, 1);
            }
            pOutVlan->outTagNum  = 1;
            pOutVlan->outVlan = tIn;
            omci_GetOutVlanVid(&pOutVlan->outVlan, tIn, fIn, fOut);

        }else
        /*insert 1 tag (x), C-F->X-C-F*/
        {
            omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tIn.tpid, pOutVlan);
            pSVlanAct->vlanAct = VLAN_ACT_ADD;
            pSVlanAct->priAct  = PRI_ACT_ASSIGN;
            pSVlanAct->assignVlan = tIn;
            omci_GetExtVlanVidAct(pSVlanAct, tIn, fIn, fOut, 1);

            pOutVlan->outVlan = tIn;
            pOutVlan->outTagNum  = 2;
            omci_GetOutVlanVid(&pOutVlan->outVlan, tIn, fIn, fOut);
        }
    }else if(removeTag)
    /*Modify and insert tag, C-F -> Y-X-F*/
    {
        omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tOut.tpid, pOutVlan);

        if (FAL_OK != feature_api(FEATURE_API_BDP_00000020, &tOut.vid, &tIn.vid))
        {
            pSVlanAct->vlanAct = VLAN_ACT_ADD;
            pSVlanAct->vidAct  = VID_ACT_ASSIGN;
            omci_GetExtVlanPriAct(pSVlanAct, tOut, 1);
            omci_GetExtVlanVidAct(pSVlanAct, tOut, fIn, fOut, 1);
        }
        pCVlanAct->vlanAct = VLAN_ACT_MODIFY;
        omci_GetExtVlanPriAct(pCVlanAct, tIn, 1);
        omci_GetExtVlanVidAct(pCVlanAct, tIn, fIn, fOut, 1);
        pCVlanAct->assignVlan.tpid = tIn.tpid;

        if (FAL_OK != feature_api(FEATURE_API_BDP_00000020, &tOut.vid, &tIn.vid))
            pOutVlan->outTagNum  = 2;
        else
            pOutVlan->outTagNum  = 1;
        pOutVlan->outVlan = tOut;
        omci_GetOutVlanVid(&pOutVlan->outVlan, tOut, fIn, fOut);

    }else
    /*Insert 2 tags (X,Y), C-F->Y-X-C-F*/
    {
        omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tOut.tpid, pOutVlan);
        pSVlanAct->vlanAct = VLAN_ACT_ADD;
        pSVlanAct->vidAct  = VID_ACT_ASSIGN;
        pSVlanAct->priAct  = PRI_ACT_ASSIGN;
        pSVlanAct->assignVlan = tOut;
        pCVlanAct->vlanAct = VLAN_ACT_ADD;
        pCVlanAct->priAct  = PRI_ACT_ASSIGN;
        pCVlanAct->vidAct  = VID_ACT_ASSIGN;
        pCVlanAct->assignVlan = tIn;
        pOutVlan->outTagNum  = 3;
        pOutVlan->outVlan = tOut;
    }

    return GOS_OK;
}


static GOS_ERROR_CODE omci_GetExtVlanDoubleTagAct(
unsigned int inputTpid,
unsigned int outputTpid,
OMCI_VLAN_ts fOut,
OMCI_VLAN_ts fIn,
OMCI_VLAN_ts tOut,
OMCI_VLAN_ts tIn,
OMCI_VLAN_ACT_ts *pSVlanAct,
OMCI_VLAN_ACT_ts *pCVlanAct,
unsigned int removeTag,
OMCI_VLAN_OUT_ts *pOutVlan)
{

    /*action*/
    if(removeTag==0)
    {
        if(tOut.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG)
        {
            pSVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;

            if(tIn.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG)
            {
                pCVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
                pOutVlan->outTagNum  = 2;
                pOutVlan->outVlan = fOut;

                if (fOut.vid == EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER &&
                        fOut.pri == EVTOCD_TBL_FILTER_PRI_DEFAULT_TAG_RULE &&
                        fIn.vid == EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER &&
                        fIn.pri == EVTOCD_TBL_FILTER_PRI_DEFAULT_TAG_RULE)
                    pOutVlan->isDefaultRule = 1;
            }else if (tIn.pri == EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_OUTER)
            /*Insert 1 tag (X), copy external priority, S-C-F->X-S-C-F*/
            {
                omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tIn.tpid, pOutVlan);
                pSVlanAct->vlanAct = VLAN_ACT_ADD;
                pSVlanAct->vidAct = VID_ACT_ASSIGN;
                pSVlanAct->priAct = PRI_ACT_COPY_OUTER;
                pSVlanAct->assignVlan = tIn;
                pOutVlan->outTagNum  = 3;
                pOutVlan->outVlan = tIn;
                pOutVlan->outVlan.pri = fOut.pri;
            }else if (tIn.pri == EVTOCD_TBL_TREATMENT_PRI_DERIVE_FROM_DSCP)
            /*Insert 1 tag (X), priority from DSCP, S-C-F->X-S-C-F*/
            {
                omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tIn.tpid, pOutVlan);
                pSVlanAct->vlanAct = VLAN_ACT_ADD;
                pSVlanAct->vidAct = VID_ACT_ASSIGN;
                pSVlanAct->priAct = PRI_ACT_FROM_DSCP;
                pSVlanAct->assignVlan = tIn;
                pOutVlan->outTagNum  = 3;
                pOutVlan->outVlan = tIn;
                pOutVlan->outVlan.pri = fOut.pri;
            }else
            /*Insert 1 tag (X), S-C-F->X-S-C-F*/
            {
                omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tIn.tpid, pOutVlan);
                pSVlanAct->vlanAct = VLAN_ACT_ADD;
                pSVlanAct->vidAct = VID_ACT_ASSIGN;
                pSVlanAct->priAct = PRI_ACT_ASSIGN;
                pSVlanAct->assignVlan = tIn;
                pOutVlan->outTagNum  = 3;
                pOutVlan->outVlan = tIn;
            }
        }
        else if(tOut.pri == EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_OUTER)
        {
            pSVlanAct->vlanAct = VLAN_ACT_ADD;
            pSVlanAct->vidAct = VID_ACT_ASSIGN;
            pSVlanAct->priAct = PRI_ACT_COPY_OUTER;
            pSVlanAct->assignVlan = tOut;
            pOutVlan->outVlan = tOut;
            pOutVlan->outVlan.pri = fOut.pri;
            omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tOut.tpid, pOutVlan);

            /*Insert 2 tag (X,Y), copy external and internal priority, S-C-F->X-Y-S-C-F*/
            if(tIn.pri == EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_INNER)
            {
                pSVlanAct->vlanAct = VLAN_ACT_ADD;
                pSVlanAct->vidAct = VID_ACT_ASSIGN;
                pSVlanAct->priAct = PRI_ACT_COPY_INNER;
                pSVlanAct->assignVlan = tIn;
                pOutVlan->outTagNum  = 4;
            }else if(tIn.pri == EVTOCD_TBL_TREATMENT_PRI_DERIVE_FROM_DSCP)
            {
                pSVlanAct->vlanAct = VLAN_ACT_ADD;
                pSVlanAct->vidAct = VID_ACT_ASSIGN;
                pSVlanAct->priAct = PRI_ACT_FROM_DSCP;
                pSVlanAct->assignVlan = tIn;
                pOutVlan->outTagNum  = 4;
            }else
            /*Insert 2 tag (X,Y), copy external and assign internal priority, S-C-F->X-Y-S-C-F*/
            {
                pSVlanAct->vlanAct = VLAN_ACT_ADD;
                pSVlanAct->vidAct = VID_ACT_ASSIGN;
                pSVlanAct->priAct = PRI_ACT_ASSIGN;
                pSVlanAct->assignVlan = tIn;
                pOutVlan->outTagNum  = 4;
            }

        }
        else
        /*Insert 2 tag (X,Y), S-C-F->X-Y-S-C-F*/
        {
            omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tOut.tpid, pOutVlan);
            pSVlanAct->vlanAct = VLAN_ACT_ADD;
            pSVlanAct->vidAct = VID_ACT_ASSIGN;
            pSVlanAct->priAct = PRI_ACT_ASSIGN;
            pSVlanAct->assignVlan = tOut;
            pCVlanAct->vlanAct = VLAN_ACT_ADD;
            pCVlanAct->vidAct = VID_ACT_ASSIGN;
            pCVlanAct->priAct = PRI_ACT_COPY_OUTER;
            pCVlanAct->assignVlan = tIn;
            pOutVlan->outTagNum  = 4;
            pOutVlan->outVlan = tOut;
        }
    }
    else if(removeTag==1)
    {
        if(tOut.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG)
        {
            /*Remove outer tag, S-C-F -> C-F*/
            if(tIn.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG)
            {
                omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, fIn.tpid, pOutVlan);
                pSVlanAct->vlanAct = VLAN_ACT_REMOVE;
                pCVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
                pOutVlan->outTagNum  = 1;
                pOutVlan->outVlan = fIn;
            }
            else if(tIn.pri == EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_OUTER)
            /*Modify external tag, keep original priority, S-C-F -> X-C-F*/
            {
                omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tIn.tpid, pOutVlan);
                if(EVTOCD_TBL_TREATMENT_VID_COPY_FROM_OUTER == tIn.vid)
                {
                    pSVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
                }
                else
                {
                    pSVlanAct->vlanAct = VLAN_ACT_MODIFY;
                    omci_GetExtVlanPriAct(pSVlanAct, tIn, 2);
                    omci_GetExtVlanVidAct(pSVlanAct, tIn, fIn, fOut , 2);
                    pSVlanAct->assignVlan.tpid = tIn.tpid;
                }
                pCVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
                pOutVlan->outTagNum = 2;
                pOutVlan->outVlan = tIn;
                omci_GetOutVlanVid(&pOutVlan->outVlan, tIn, fIn, fOut);
                pOutVlan->outVlan.pri = fOut.pri;
            }else if(tIn.pri == EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_INNER)
            /*Modify external tag, copy priority from inner, S-C-F -> X-C-F*/
            {
                omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tIn.tpid, pOutVlan);
                pSVlanAct->vlanAct = VLAN_ACT_MODIFY;
                omci_GetExtVlanPriAct(pSVlanAct, tIn, 2);
                omci_GetExtVlanVidAct(pSVlanAct, tIn, fIn, fOut, 2);
                pSVlanAct->assignVlan.tpid = tIn.tpid;

                pCVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;

                pOutVlan->outTagNum = 2;
                pOutVlan->outVlan = tIn;
                omci_GetOutVlanVid(&pOutVlan->outVlan, tIn, fIn, fOut);
                pOutVlan->outVlan.pri = fIn.pri;
            }
            else if(tIn.pri == EVTOCD_TBL_TREATMENT_PRI_DERIVE_FROM_DSCP)
            /*Modify external tag, priority from DSCP, S-C-F -> X-C-F*/
            {
                omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tIn.tpid, pOutVlan);
                pSVlanAct->vlanAct = VLAN_ACT_MODIFY;
                omci_GetExtVlanPriAct(pSVlanAct, tIn, 2);
                omci_GetExtVlanVidAct(pSVlanAct, tIn, fIn, fOut, 2);
                pSVlanAct->assignVlan.tpid = tIn.tpid;

                pCVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;

                pOutVlan->outTagNum = 2;
                pOutVlan->outVlan = tIn;
                omci_GetOutVlanVid(&pOutVlan->outVlan, tIn, fIn, fOut);
                pOutVlan->outVlan.pri = fOut.pri;
            }
            else
            /*Modify external tag, S-C-F -> X-C-F*/
            {
                omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tIn.tpid, pOutVlan);
                pSVlanAct->vlanAct = VLAN_ACT_MODIFY;
                omci_GetExtVlanPriAct(pSVlanAct, tIn, 2);
                omci_GetExtVlanVidAct(pSVlanAct, tIn, fIn, fOut, 2);
                pSVlanAct->assignVlan.tpid = tIn.tpid;

                pCVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;

                pOutVlan->outTagNum  = 2;
                pOutVlan->outVlan = tIn;
                omci_GetOutVlanVid(&pOutVlan->outVlan, tIn, fIn, fOut);
            }
        }
    }else
    {
        /*Remove both tags, S-C-F -> F*/
        if(tOut.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG && tIn.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG)
        {
            pSVlanAct->vlanAct = VLAN_ACT_REMOVE;
            pCVlanAct->vlanAct = VLAN_ACT_REMOVE;
            pOutVlan->outTagNum = 0;

        }else if(tOut.pri == EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_INNER && tOut.vid == EVTOCD_TBL_TREATMENT_VID_COPY_FROM_INNER &&
        tIn.pri == EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_OUTER && tIn.vid == EVTOCD_TBL_TREATMENT_VID_COPY_FROM_OUTER)
        /*Swap both tags, S-C-F->C-S-F*/
        {
            omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tOut.tpid, pOutVlan);
            pSVlanAct->vlanAct = VLAN_ACT_MODIFY;
            pSVlanAct->vidAct = VID_ACT_COPY_INNER;
            pSVlanAct->priAct = PRI_ACT_COPY_INNER;
            pCVlanAct->vlanAct = VLAN_ACT_MODIFY;
            pCVlanAct->vidAct = VID_ACT_COPY_OUTER;
            pCVlanAct->priAct = PRI_ACT_COPY_OUTER;
            pOutVlan->outTagNum  = 2;
            pOutVlan->outVlan = fIn;
        }else if(tOut.pri == EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_OUTER)
        {

            pSVlanAct->vlanAct = VLAN_ACT_MODIFY;
            pSVlanAct->priAct = PRI_ACT_COPY_OUTER;
            omci_GetExtVlanVidAct(pSVlanAct, tOut, fIn, fOut, 2);
            pSVlanAct->assignVlan.tpid = tOut.tpid;
            pOutVlan->outVlan = tOut;
            omci_GetOutVlanVid(&pOutVlan->outVlan, tOut, fIn, fOut);
            pOutVlan->outVlan.pri = fOut.pri;
            omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tOut.tpid, pOutVlan);


            /*Modify both tags, keep original priority, S-C-F -> Y-X-F*/
            if(tIn.pri == EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_INNER)
            {
                pCVlanAct->vlanAct = VLAN_ACT_MODIFY;
                pCVlanAct->priAct = PRI_ACT_COPY_INNER;
                omci_GetExtVlanVidAct(pCVlanAct, tIn, fIn, fOut, 2);
                pCVlanAct->assignVlan.tpid = tIn.tpid;
                pOutVlan->outTagNum  = 2;

            }else if(tIn.pri == EVTOCD_TBL_TREATMENT_PRI_DERIVE_FROM_DSCP)
            /*Modify both tags, inner priority from DSCP, S-C-F -> Y-X-F*/
            {
                pCVlanAct->vlanAct = VLAN_ACT_MODIFY;
                pCVlanAct->priAct = PRI_ACT_FROM_DSCP;
                omci_GetExtVlanVidAct(pCVlanAct, tIn, fIn, fOut, 2);
                pCVlanAct->assignVlan.tpid = tIn.tpid;
                pOutVlan->outTagNum  = 2;

            }else
            /*Modify both tags, keep original external priority, S-C-F -> Y-X-F*/
            {
                pCVlanAct->vlanAct = VLAN_ACT_MODIFY;
                pCVlanAct->priAct = PRI_ACT_ASSIGN;
                pCVlanAct->assignVlan.pri = tIn.pri;
                omci_GetExtVlanVidAct(pCVlanAct, tIn, fIn, fOut, 2);
                pCVlanAct->assignVlan.tpid = tIn.tpid;
                pOutVlan->outTagNum  = 2;
            }
        }else
        /*Modify both tags, S-C-F->Y-X-F*/
        {
            omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tOut.tpid, pOutVlan);
            pSVlanAct->vlanAct = VLAN_ACT_MODIFY;
            pSVlanAct->priAct = PRI_ACT_ASSIGN;
            pSVlanAct->assignVlan.pri = tOut.pri;
            omci_GetExtVlanVidAct(pSVlanAct, tOut, fIn, fOut, 2);
            pSVlanAct->assignVlan.tpid = tOut.tpid;
            pCVlanAct->vlanAct = VLAN_ACT_MODIFY;
            pCVlanAct->priAct = PRI_ACT_ASSIGN;
            pCVlanAct->assignVlan.pri = tIn.pri;
            omci_GetExtVlanVidAct(pCVlanAct, tIn, fIn, fOut, 2);
            pCVlanAct->assignVlan.tpid = tIn.tpid;
            pCVlanAct->assignVlan = tIn;
            pOutVlan->outTagNum  = 2;
            pOutVlan->outVlan = tOut;
            omci_GetOutVlanVid(&pOutVlan->outVlan, tOut, fIn, fOut);
        }
    }
    return GOS_OK;
}

static void omci_vlan_xslateEthType(unsigned int *pDstType, unsigned int meType)
{
    if (!pDstType)
        return;

    switch (meType)
    {
        case EVTOCD_TBL_FILTER_ET_IPOE_0800:
            *pDstType = ETHTYPE_FILTER_IP;
            break;
        case EVTOCD_TBL_FILTER_ET_PPPOE_8863_8864:
            *pDstType = ETHTYPE_FILTER_PPPOE;
            break;
        case EVTOCD_TBL_FILTER_ET_ARP_0806:
            *pDstType = ETHTYPE_FILTER_ARP;
            break;
        case EVTOCD_TBL_FILTER_ET_IPV6_86DD:
            *pDstType = ETHTYPE_FILTER_IPV6;
            break;
        case EVTOCD_TBL_FILTER_ET_DO_NOT_FILTER:
        default:
            *pDstType = ETHTYPE_FILTER_NO_CARE;
            break;
    }
}

/*follow G.984.4 page 115*/
static GOS_ERROR_CODE omci_GetExtVlanRule(
unsigned int inputTpid,
unsigned int outputTpid,
OMCI_VLAN_ts fOut,
OMCI_VLAN_ts fIn,
OMCI_VLAN_ts tOut,
OMCI_VLAN_ts tIn,
unsigned removeTag,
OMCI_VLAN_OPER_ts *pRule)
{
    OMCI_VLAN_FILTER_ts *pVlanFilter = &pRule->filterRule;
    OMCI_VLAN_ACT_ts *pSVlanAct = &pRule->sTagAct;
    OMCI_VLAN_ACT_ts *pCVlanAct = &pRule->cTagAct;

    if(fOut.pri == EVTOCD_TBL_FILTER_PRI_IGNORE_OTHER_FIELD)
    {
        pVlanFilter->filterStagMode = VLAN_FILTER_NO_TAG;
        /*untag frames*/
        if(fIn.pri == EVTOCD_TBL_FILTER_PRI_IGNORE_OTHER_FIELD)
        {
            if(pVlanFilter->etherType == (OMCI_ETHTYPE_FILTER_MODE_e)EVTOCD_TBL_FILTER_ET_DO_NOT_FILTER)
                pVlanFilter->filterCtagMode = VLAN_FILTER_NO_TAG;
            else
                pVlanFilter->filterCtagMode = (VLAN_FILTER_ETHTYPE | VLAN_FILTER_NO_TAG);
            if(EVTOCD_TBL_TREATMENT_DISCARD_FRAME == removeTag)
                pRule->outStyle.isDefaultRule = OMCI_EXTVLAN_REMOVE_TAG_DISCARD;
            else
                omci_GetExtVlanUntagAct(inputTpid, outputTpid, fOut, fIn, tOut,tIn,pSVlanAct,pCVlanAct,&pRule->outStyle);
        }
        /*single frames*/
        else
        {
            if(fIn.pri == EVTOCD_TBL_FILTER_PRI_DEFAULT_TAG_RULE)
            {
                /*default case, do nothing, C-F->C-F*/
                pVlanFilter->filterCtagMode = VLAN_FILTER_CARE_TAG;
            }
            else
            {
                if(fIn.pri == EVTOCD_TBL_FILTER_PRI_DO_NOT_FILTER)
                {
                    if(fIn.vid == EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER)
                    {
                        if(pVlanFilter->etherType == (OMCI_ETHTYPE_FILTER_MODE_e)EVTOCD_TBL_FILTER_ET_DO_NOT_FILTER)
                        {
                            if(fIn.tpid <= EVTOCD_TBL_FILTER_TPID_8100 ||
                                (fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID && OMCI_GET_TPID_VALUE(inputTpid) == TPID_8100))
                            {
                                pVlanFilter->filterCtagMode = VLAN_FILTER_CARE_TAG;
                            }
                            else if(fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID &&
                                (OMCI_GET_TPID_VALUE(inputTpid) == TPID_88A8 ||
                                 OMCI_GET_TPID_VALUE(inputTpid) == TPID_9100))
                            {
                                pVlanFilter->filterCtagMode = VLAN_FILTER_NO_TAG;
                                pVlanFilter->filterStagMode = VLAN_FILTER_CARE_TAG;
                            }
                        }else
                        {
                            if(fIn.tpid <= EVTOCD_TBL_FILTER_TPID_8100 ||
                                (fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID && OMCI_GET_TPID_VALUE(inputTpid) == TPID_8100))
                            {
                                pVlanFilter->filterCtagMode = VLAN_FILTER_ETHTYPE | VLAN_FILTER_CARE_TAG;
                            }
                            else if(fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID &&
                                (OMCI_GET_TPID_VALUE(inputTpid) == TPID_88A8 ||
                                 OMCI_GET_TPID_VALUE(inputTpid) == TPID_9100))
                            {
                                pVlanFilter->filterCtagMode = VLAN_FILTER_NO_TAG;
                                pVlanFilter->filterStagMode = VLAN_FILTER_ETHTYPE | VLAN_FILTER_CARE_TAG;
                            }
                        }
                    }
                    else
                    {
                        if(fIn.tpid <= EVTOCD_TBL_FILTER_TPID_8100 ||
                            (fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID && OMCI_GET_TPID_VALUE(inputTpid) == TPID_8100))
                        {
                            if(pVlanFilter->etherType == (OMCI_ETHTYPE_FILTER_MODE_e)EVTOCD_TBL_FILTER_ET_DO_NOT_FILTER)
                                pVlanFilter->filterCtagMode = VLAN_FILTER_VID;
                            else
                                pVlanFilter->filterCtagMode = (VLAN_FILTER_ETHTYPE | VLAN_FILTER_VID);
                            pVlanFilter->filterCTag = fIn;
                        }
                        else if(fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID &&
                            (OMCI_GET_TPID_VALUE(inputTpid) == TPID_88A8 ||
                             OMCI_GET_TPID_VALUE(inputTpid) == TPID_9100))
                        {
                            pVlanFilter->filterCtagMode = VLAN_FILTER_NO_TAG;
                            if(pVlanFilter->etherType == (OMCI_ETHTYPE_FILTER_MODE_e)EVTOCD_TBL_FILTER_ET_DO_NOT_FILTER)
                                pVlanFilter->filterStagMode = VLAN_FILTER_VID;
                            else
                                pVlanFilter->filterStagMode = (VLAN_FILTER_ETHTYPE | VLAN_FILTER_VID);
                            pVlanFilter->filterSTag = fIn;
                        }
                    }
                }else
                {
                    if(fIn.tpid <= EVTOCD_TBL_FILTER_TPID_8100 ||
                        (fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID && OMCI_GET_TPID_VALUE(inputTpid) == TPID_8100))
                    {
                        if(fIn.vid == EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER)
                            pVlanFilter->filterCtagMode = VLAN_FILTER_PRI;
                        else
                            pVlanFilter->filterCtagMode = VLAN_FILTER_TCI;

                        if(pVlanFilter->etherType != (OMCI_ETHTYPE_FILTER_MODE_e)EVTOCD_TBL_FILTER_ET_DO_NOT_FILTER)
                            pVlanFilter->filterCtagMode |= VLAN_FILTER_ETHTYPE;

                        pVlanFilter->filterCTag = fIn;
                    }
                    else if(fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID &&
                        (OMCI_GET_TPID_VALUE(inputTpid) == TPID_88A8 ||
                         OMCI_GET_TPID_VALUE(inputTpid) == TPID_9100))
                    {
                        pVlanFilter->filterCtagMode = VLAN_FILTER_NO_TAG;
                        if(fIn.vid == EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER)
                            pVlanFilter->filterStagMode = VLAN_FILTER_PRI;
                        else
                            pVlanFilter->filterStagMode = VLAN_FILTER_TCI;

                        if(pVlanFilter->etherType != (OMCI_ETHTYPE_FILTER_MODE_e)EVTOCD_TBL_FILTER_ET_DO_NOT_FILTER)
                            pVlanFilter->filterStagMode |= VLAN_FILTER_ETHTYPE;

                        pVlanFilter->filterSTag = fIn;
                    }
                }
            }
            if(EVTOCD_TBL_TREATMENT_DISCARD_FRAME == removeTag)
                pRule->outStyle.isDefaultRule = OMCI_EXTVLAN_REMOVE_TAG_DISCARD;
            else
                omci_GetExtVlanSingleTagAct(inputTpid, outputTpid, fOut, fIn, tOut, tIn, pSVlanAct, pCVlanAct, removeTag,&pRule->outStyle);
        }
    }
    /*double frames*/
    else
    {
        /*default case, do nothing, S-C-F->S-C-F*/
        if(fOut.pri == EVTOCD_TBL_FILTER_PRI_DEFAULT_TAG_RULE && fIn.pri == EVTOCD_TBL_FILTER_PRI_DEFAULT_TAG_RULE)
        {
            pVlanFilter->filterStagMode = VLAN_FILTER_CARE_TAG;
            pVlanFilter->filterCtagMode = VLAN_FILTER_CARE_TAG;
        }else
        {
            /*filter rule*/
            if(fOut.pri == EVTOCD_TBL_FILTER_PRI_DO_NOT_FILTER)
            {
                if ((fOut.tpid <= EVTOCD_TBL_FILTER_TPID_8100 ||
                    (fOut.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID && OMCI_GET_TPID_VALUE(inputTpid) == TPID_8100)) &&
                    (fIn.tpid <= EVTOCD_TBL_FILTER_TPID_8100 ||
                    (fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID && OMCI_GET_TPID_VALUE(inputTpid) == TPID_8100)))
                {
                    if (EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER != fOut.vid)
                        pVlanFilter->filterStagMode = VLAN_FILTER_VID;
                    pVlanFilter->filterSTag = fOut;
                }
                else if(fOut.tpid <= EVTOCD_TBL_FILTER_TPID_8100 ||
                    (fOut.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID && OMCI_GET_TPID_VALUE(inputTpid) == TPID_8100))
                {
                    if (EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER != fOut.vid)
                        pVlanFilter->filterCtagMode = VLAN_FILTER_VID;
                    pVlanFilter->filterCTag = fOut;
                }
                else if(fOut.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID &&
                    (OMCI_GET_TPID_VALUE(inputTpid) == TPID_88A8 ||
                     OMCI_GET_TPID_VALUE(inputTpid) == TPID_9100))
                {
                    if (EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER != fOut.vid)
                        pVlanFilter->filterStagMode = VLAN_FILTER_VID;
                    pVlanFilter->filterSTag = fOut;
                }
            }
            else
            {

                if ((fOut.tpid <= EVTOCD_TBL_FILTER_TPID_8100 ||
                    (fOut.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID && OMCI_GET_TPID_VALUE(inputTpid) == TPID_8100)) &&
                    (fIn.tpid <= EVTOCD_TBL_FILTER_TPID_8100 ||
                    (fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID && OMCI_GET_TPID_VALUE(inputTpid) == TPID_8100)))
                {
                    if(fOut.vid == EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER)
                        pVlanFilter->filterStagMode = VLAN_FILTER_PRI;
                    else
                        pVlanFilter->filterStagMode = VLAN_FILTER_TCI;
                    pVlanFilter->filterSTag = fOut;
                }
                else if(fOut.tpid <= EVTOCD_TBL_FILTER_TPID_8100 ||
                    (fOut.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID && OMCI_GET_TPID_VALUE(inputTpid) == TPID_8100))
                {
                    if(fOut.vid == EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER)
                        pVlanFilter->filterCtagMode = VLAN_FILTER_PRI;
                    else
                        pVlanFilter->filterCtagMode = VLAN_FILTER_TCI;
                    pVlanFilter->filterCTag = fOut;
                }
                else
                {
                    if(fOut.vid == EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER)
                        pVlanFilter->filterStagMode = VLAN_FILTER_PRI;
                    else
                        pVlanFilter->filterStagMode = VLAN_FILTER_TCI;
                    pVlanFilter->filterSTag = fOut;
                }
            }

            if(fIn.pri == EVTOCD_TBL_FILTER_PRI_DO_NOT_FILTER)
            {
                if(fIn.tpid <= EVTOCD_TBL_FILTER_TPID_8100 ||
                    (fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID && OMCI_GET_TPID_VALUE(inputTpid) == TPID_8100))
                {
                    if(fIn.vid == EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER)
                        pVlanFilter->filterCtagMode = VLAN_FILTER_CARE_TAG;
                    else
                        pVlanFilter->filterCtagMode = VLAN_FILTER_VID;

                    if(pVlanFilter->etherType != (OMCI_ETHTYPE_FILTER_MODE_e)EVTOCD_TBL_FILTER_ET_DO_NOT_FILTER)
                        pVlanFilter->filterCtagMode |= VLAN_FILTER_ETHTYPE;

                    pVlanFilter->filterCTag = fIn;
                }
                else if(fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID &&
                    (OMCI_GET_TPID_VALUE(inputTpid) == TPID_88A8 ||
                     OMCI_GET_TPID_VALUE(inputTpid) == TPID_9100))
                {
                    if(fIn.vid == EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER)
                        pVlanFilter->filterStagMode = VLAN_FILTER_CARE_TAG;
                    else
                        pVlanFilter->filterStagMode = VLAN_FILTER_VID;

                    if(pVlanFilter->etherType != (OMCI_ETHTYPE_FILTER_MODE_e)EVTOCD_TBL_FILTER_ET_DO_NOT_FILTER)
                        pVlanFilter->filterStagMode |= VLAN_FILTER_ETHTYPE;

                    pVlanFilter->filterSTag = fIn;
                }
            }
            else
            {

                if(fIn.tpid <= EVTOCD_TBL_FILTER_TPID_8100 ||
                    (fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID && OMCI_GET_TPID_VALUE(inputTpid) == TPID_8100))
                {
                    if(fIn.vid == EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER)
                        pVlanFilter->filterCtagMode = VLAN_FILTER_PRI;
                    else
                        pVlanFilter->filterCtagMode = VLAN_FILTER_TCI;

                    if(pVlanFilter->etherType != (OMCI_ETHTYPE_FILTER_MODE_e)EVTOCD_TBL_FILTER_ET_DO_NOT_FILTER)
                        pVlanFilter->filterCtagMode |= VLAN_FILTER_ETHTYPE;

                    pVlanFilter->filterCTag = fIn;
                }
                else if(fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID &&
                    (OMCI_GET_TPID_VALUE(inputTpid) == TPID_88A8 ||
                    OMCI_GET_TPID_VALUE(inputTpid) == TPID_9100))
                {
                    if(fIn.vid == EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER)
                        pVlanFilter->filterStagMode = VLAN_FILTER_PRI;
                    else
                        pVlanFilter->filterStagMode = VLAN_FILTER_TCI;

                    if(pVlanFilter->etherType != (OMCI_ETHTYPE_FILTER_MODE_e)EVTOCD_TBL_FILTER_ET_DO_NOT_FILTER)
                        pVlanFilter->filterStagMode |= VLAN_FILTER_ETHTYPE;

                    pVlanFilter->filterSTag = fIn;
                }

            }

        }
        if(EVTOCD_TBL_TREATMENT_DISCARD_FRAME == removeTag)
            pRule->outStyle.isDefaultRule = OMCI_EXTVLAN_REMOVE_TAG_DISCARD;
        else
            omci_GetExtVlanDoubleTagAct(inputTpid, outputTpid, fOut, fIn, tOut,tIn, pSVlanAct, pCVlanAct, removeTag, &pRule->outStyle);
    }

    omci_vlan_xslateEthType(&pVlanFilter->etherType, pVlanFilter->etherType);

    return GOS_OK;
}

/*follow G.988.201210 page 137*/
static GOS_ERROR_CODE omci_GetExtVlanRuleByDsDir(
unsigned int inputTpid,
unsigned int outputTpid,
OMCI_VLAN_ts fOut,
OMCI_VLAN_ts fIn,
OMCI_VLAN_ts tOut,
OMCI_VLAN_ts tIn,
unsigned removeTag,
OMCI_VLAN_OPER_ts *pRule)
{
    OMCI_VLAN_FILTER_ts *pVlanFilter = &pRule->filterRule;
    OMCI_VLAN_ACT_ts *pSVlanAct = &pRule->sTagAct;
    OMCI_VLAN_ACT_ts *pCVlanAct = &pRule->cTagAct;
    OMCI_VLAN_OUT_ts *pOutVlan = &pRule->outStyle;

    // get outStyle.tpid = ext vlan treat the first tpid result
    omci_GetExtVlanTpidAct(inputTpid, fOut.tpid, fIn.tpid, outputTpid, tIn.tpid, pOutVlan);

    if(fOut.pri == EVTOCD_TBL_FILTER_PRI_IGNORE_OTHER_FIELD)
    {
        /*untag frames*/
        if(fIn.pri == EVTOCD_TBL_FILTER_PRI_IGNORE_OTHER_FIELD)
        {
            pOutVlan->outTagNum= 0; // outstyle is no meaning
            /*default case, do nothing, F->F*/
            if(tOut.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG &&
                tIn.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG)
            {
                pVlanFilter->filterStagMode = VLAN_FILTER_NO_TAG;
                pVlanFilter->filterCtagMode = VLAN_FILTER_NO_TAG;
                pSVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
                pCVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
                if (0 == removeTag)
                    pOutVlan->isDefaultRule = 1;

            }
            else if(tOut.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG)
            /*insert 1 full tag (X), F->X-F */
            {
                if(TPID_88A8 == pOutVlan->tpid || TPID_9100 == pOutVlan->tpid)
                {
                    pVlanFilter->filterStagMode = VLAN_FILTER_TCI;
                    pVlanFilter->filterCtagMode = VLAN_FILTER_NO_TAG;
                    pVlanFilter->filterSTag = tIn;
                    pSVlanAct->vlanAct = VLAN_ACT_REMOVE;
                    pCVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
                }
                else if (TPID_8100 == pOutVlan->tpid)
                {
                    pVlanFilter->filterStagMode = VLAN_FILTER_NO_TAG;
                    pVlanFilter->filterCtagMode = VLAN_FILTER_TCI;
                    pVlanFilter->filterCTag = tIn;
                    pSVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
                    pCVlanAct->vlanAct = VLAN_ACT_REMOVE;
                }
            }
            else
            /*insert 2 full tag (X,Y), F->Y-X-F*/
            {
                pVlanFilter->filterStagMode = VLAN_FILTER_TCI;
                pVlanFilter->filterSTag = tOut;
                pVlanFilter->filterCtagMode = VLAN_FILTER_TCI;
                pVlanFilter->filterCTag = tIn;
                pSVlanAct->vlanAct = VLAN_ACT_REMOVE;
                pCVlanAct->vlanAct = VLAN_ACT_REMOVE;
            }
        }
        /*single frames*/
        else
        {
            pOutVlan->outTagNum= 1;

            // default case, do nothing, C-F->C-F
            if(fIn.pri == EVTOCD_TBL_FILTER_PRI_DEFAULT_TAG_RULE &&
                fIn.vid == EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER &&
                tIn.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG &&
                fIn.vid == fOut.vid && tIn.pri == tOut.pri && 0 == removeTag)
            {
                if(TPID_88A8 == pOutVlan->tpid || TPID_9100 == pOutVlan->tpid)
                {
                    pVlanFilter->filterStagMode = VLAN_FILTER_CARE_TAG;
                    pVlanFilter->filterCtagMode = VLAN_FILTER_NO_TAG;
                }
                else if (TPID_8100 == pOutVlan->tpid)
                {
                    pVlanFilter->filterStagMode = VLAN_FILTER_NO_TAG;
                    pVlanFilter->filterCtagMode = VLAN_FILTER_CARE_TAG;
                }

                pSVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
                pCVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
                pOutVlan->isDefaultRule = 1;

            }
            else
            {
                if (0 == removeTag)
                {
                    //Insert 1 tag (X),copy priority: C-F->X-CF
                    if (tOut.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG &&
                        tIn.pri == EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_INNER)
                    {
                        if (fIn.tpid <= EVTOCD_TBL_FILTER_TPID_8100 ||
                            ((fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID &&
                            OMCI_GET_TPID_VALUE(inputTpid) == TPID_8100) &&
                            TPID_8100 == pOutVlan->tpid))
                        {
                            pVlanFilter->filterStagMode = VLAN_FILTER_VID;
                            pVlanFilter->filterSTag = tIn;
                            if(tIn.vid == EVTOCD_TBL_TREATMENT_VID_COPY_FROM_INNER)
                                pVlanFilter->filterSTag.vid = fIn.vid;
                        }
                        else if (fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID &&
                            (OMCI_GET_TPID_VALUE(inputTpid) == TPID_88A8 ||
                             OMCI_GET_TPID_VALUE(inputTpid) == TPID_9100))
                        {
                            pVlanFilter->filterCtagMode = VLAN_FILTER_VID;
                            pVlanFilter->filterCTag = fIn;
                        }
                        pSVlanAct->vlanAct = VLAN_ACT_REMOVE;
                        if (EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER == fIn.vid)
                        {
                            pCVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
                        }
                        else
                        {
                            pCVlanAct->vlanAct = VLAN_ACT_ADD;
                            pCVlanAct->assignVlan = fIn;
                            pOutVlan->outVlan = pCVlanAct->assignVlan;
                        }
                        pCVlanAct->vidAct  = VID_ACT_ASSIGN;
                        pCVlanAct->priAct= PRI_ACT_COPY_INNER;
                    }
                    // transparent orignal
                    else if (EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG == tOut.pri &&
                        tIn.pri == tOut.pri)
                    {
                        if(fIn.tpid <= EVTOCD_TBL_FILTER_TPID_8100 ||
                            (fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID && OMCI_GET_TPID_VALUE(inputTpid) == TPID_8100))
                        {
                            pVlanFilter->filterStagMode = VLAN_FILTER_NO_TAG;

                            if (EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER == fIn.vid &&
                                EVTOCD_TBL_FILTER_PRI_DO_NOT_FILTER == fIn.pri)
                            {
                                pVlanFilter->filterCtagMode = VLAN_FILTER_CARE_TAG;
                            }
                            else if (EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER != fIn.vid &&
                                EVTOCD_TBL_FILTER_PRI_DO_NOT_FILTER == fIn.pri)
                            {
                                pVlanFilter->filterCtagMode = VLAN_FILTER_VID;
                            }

                            pVlanFilter->filterCTag = fIn;
                        }
                        else if(fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID &&
                            (OMCI_GET_TPID_VALUE(inputTpid) == TPID_88A8 ||
                             OMCI_GET_TPID_VALUE(inputTpid) == TPID_9100))
                        {
                            pVlanFilter->filterCtagMode = VLAN_FILTER_NO_TAG;

                            if (EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER == fIn.vid &&
                                EVTOCD_TBL_FILTER_PRI_DO_NOT_FILTER == fIn.pri)
                            {
                                pVlanFilter->filterStagMode = VLAN_FILTER_CARE_TAG;
                            }
                            else if (EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER != fIn.vid &&
                                EVTOCD_TBL_FILTER_PRI_DO_NOT_FILTER == fIn.pri)
                            {
                                pVlanFilter->filterStagMode = VLAN_FILTER_VID;
                            }

                            pVlanFilter->filterSTag = fIn;
                        }
                        pSVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
                        pCVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
                    }
                    //Insert 1 full tag (X):C-F->X-C-F
                    else if (tOut.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG)
                    {
                        pVlanFilter->filterStagMode = VLAN_FILTER_TCI;
                        pVlanFilter->filterSTag = tIn;
                        pVlanFilter->filterCtagMode = VLAN_FILTER_VID;

                        if(tIn.vid == EVTOCD_TBL_TREATMENT_VID_COPY_FROM_INNER)
                            pVlanFilter->filterSTag.vid = fIn.vid;

                        if (fIn.pri < EVTOCD_TBL_FILTER_PRI_DO_NOT_FILTER)
                            pVlanFilter->filterCtagMode |= VLAN_FILTER_PRI;
                        pVlanFilter->filterCTag = fIn;
                        pSVlanAct->vlanAct = VLAN_ACT_REMOVE;
                        pCVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
                    }
                    //Insert 2 tags (X,Y):C-F->Y-X-C-F
                    else
                    {
                        pVlanFilter->filterStagMode = VLAN_FILTER_VID | VLAN_FILTER_PRI;
                        pVlanFilter->filterSTag = tOut;
                        pVlanFilter->filterCtagMode = VLAN_FILTER_VID | VLAN_FILTER_PRI;
                        pVlanFilter->filterCTag = tIn;
                        pSVlanAct->vlanAct = VLAN_ACT_REMOVE;
                        pCVlanAct->vlanAct = VLAN_ACT_REMOVE;
                    }
                    pOutVlan->outVlan = fIn;
                }
                else
                {
                    //Remove tag: C-F->F
                    if (tOut.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG &&
                        tOut.pri == tIn.pri)
                    {
                        pVlanFilter->filterStagMode = VLAN_FILTER_NO_TAG;
                        pVlanFilter->filterCtagMode = VLAN_FILTER_NO_TAG;

                        pSVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
                        pSVlanAct->priAct  = PRI_ACT_ASSIGN;
                        pSVlanAct->vidAct  = VID_ACT_ASSIGN;
                        memset(&pSVlanAct->assignVlan, 0, sizeof(OMCI_VLAN_ts));
                        pCVlanAct->vlanAct = VLAN_ACT_ADD;
                        pCVlanAct->priAct  = PRI_ACT_ASSIGN;
                        pCVlanAct->vidAct  = VID_ACT_ASSIGN;
                        pCVlanAct->assignVlan = fIn;
                        pOutVlan->outVlan = pCVlanAct->assignVlan;
                    }
                    //Modify tag, keep original priority/DSCP/Assign pri : C-F ->X-F
                    else if (tOut.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG)
                    {
                        if (TPID_88A8 == pOutVlan->tpid || TPID_9100 == pOutVlan->tpid)
                        {
                            pVlanFilter->filterStagMode = VLAN_FILTER_VID;
                            pVlanFilter->filterCtagMode = VLAN_FILTER_NO_TAG;
                            pVlanFilter->filterSTag = tIn;
                            if (EVTOCD_TBL_TREATMENT_VID_COPY_FROM_INNER == tIn.vid)
                                pVlanFilter->filterSTag.vid = fIn.vid;

                            if (EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_INNER > tIn.pri)
                                pVlanFilter->filterStagMode |= VLAN_FILTER_PRI;
                        }
                        else if (TPID_8100 == pOutVlan->tpid)
                        {
                            pVlanFilter->filterStagMode = VLAN_FILTER_NO_TAG;
                            pVlanFilter->filterCtagMode = VLAN_FILTER_VID;
                            pVlanFilter->filterCTag = tIn;
                            if (EVTOCD_TBL_TREATMENT_VID_COPY_FROM_INNER == tIn.vid)
                                pVlanFilter->filterCTag.vid = fIn.vid;

                            if (EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_INNER > tIn.pri)
                                pVlanFilter->filterCtagMode |= VLAN_FILTER_PRI;

                        }

                        if (fIn.tpid <= EVTOCD_TBL_FILTER_TPID_8100 ||
                            (fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID &&
                            OMCI_GET_TPID_VALUE(inputTpid) == TPID_8100))
                        {
                            pSVlanAct->vlanAct = VLAN_ACT_NON;
                            if (TPID_88A8 == pOutVlan->tpid || TPID_9100 == pOutVlan->tpid)
                                pSVlanAct->vlanAct = VLAN_ACT_REMOVE;
                            pCVlanAct->vlanAct = VLAN_ACT_MODIFY;
                            pCVlanAct->priAct  =
                                (EVTOCD_TBL_FILTER_PRI_DO_NOT_FILTER == fIn.pri ?
                                PRI_ACT_COPY_OUTER : PRI_ACT_ASSIGN);
                            pCVlanAct->vidAct  = VID_ACT_ASSIGN;
                            pCVlanAct->assignVlan = fIn;
                            pOutVlan->outVlan = pCVlanAct->assignVlan;
                        }
                        else if (fIn.tpid >= EVTOCD_TBL_FILTER_TPID_INPUT_TPID &&
                            (OMCI_GET_TPID_VALUE(inputTpid) == TPID_88A8 ||
                             OMCI_GET_TPID_VALUE(inputTpid) == TPID_9100))
                        {
                            pSVlanAct->vlanAct = VLAN_ACT_MODIFY;
                            pSVlanAct->priAct  =
                                (EVTOCD_TBL_FILTER_PRI_DO_NOT_FILTER == fIn.pri ?
                                PRI_ACT_COPY_OUTER : PRI_ACT_ASSIGN);
                            pSVlanAct->vidAct  = VID_ACT_ASSIGN;
                            pSVlanAct->assignVlan = fIn;
                            pCVlanAct->vlanAct = VLAN_ACT_NON;
                            if (TPID_8100 == pOutVlan->tpid)
                                pCVlanAct->vlanAct = VLAN_ACT_REMOVE;
                            pOutVlan->outVlan = pSVlanAct->assignVlan;
                        }
                    }
                    //Modify and insert tag: C-F->Y-X-F
                    else
                    {
                        if (FAL_OK != feature_api(FEATURE_API_BDP_00000020, &tOut.vid, &tIn.vid))
                        {
                            pVlanFilter->filterStagMode  = (tOut.pri < EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_INNER ? (VLAN_FILTER_VID | VLAN_FILTER_PRI) : VLAN_FILTER_VID);
                            pVlanFilter->filterSTag = tOut;
                            if(tOut.vid == EVTOCD_TBL_TREATMENT_VID_COPY_FROM_INNER)
                                pVlanFilter->filterSTag.vid = fIn.vid;

                        }
                        else
                        {
                            pVlanFilter->filterStagMode = VLAN_FILTER_NO_TAG;
                        }
                        pVlanFilter->filterCtagMode  = (tIn.pri < EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_INNER ? (VLAN_FILTER_VID | VLAN_FILTER_PRI) : VLAN_FILTER_VID);
                        pVlanFilter->filterCTag = tIn;
                        pSVlanAct->vlanAct = VLAN_ACT_REMOVE;
                        pCVlanAct->vlanAct = VLAN_ACT_MODIFY;
                        pCVlanAct->priAct  = (fIn.pri < EVTOCD_TBL_FILTER_PRI_DO_NOT_FILTER ? PRI_ACT_ASSIGN :
                                             (fIn.pri == EVTOCD_TBL_FILTER_PRI_DO_NOT_FILTER ? PRI_ACT_COPY_OUTER : PRI_ACT_TRANSPARENT));
                        pCVlanAct->vidAct  = VID_ACT_ASSIGN;
                        pCVlanAct->assignVlan = fIn;
                        pOutVlan->outVlan = pCVlanAct->assignVlan;
                    }
                }
            }
        }
    }
    /*double frames*/
    else
    {
        pOutVlan->outTagNum= 2;

        /*default case, do nothing, S-C-F->S-C-F*/
        if (fOut.pri == EVTOCD_TBL_FILTER_PRI_DEFAULT_TAG_RULE &&
            fOut.vid == EVTOCD_TBL_FILTER_VID_DO_NOT_FILTER &&
            tOut.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG &&
            fIn.pri == fOut.pri && fIn.vid == fOut.vid &&
            tIn.pri == tOut.pri && 0 == removeTag)
        {

            pVlanFilter->filterStagMode = VLAN_FILTER_CARE_TAG;
            pVlanFilter->filterCtagMode = VLAN_FILTER_CARE_TAG;
            pSVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
            pCVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
            pOutVlan->isDefaultRule = 1;
        }
        else
        {
            if (0 == removeTag)
            {
                //Insert 1 tag (X), copy external priority: S-C-F->X-S-C-F
                if (tOut.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG &&
                    tIn.pri == EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_OUTER)
                {
                    if (fOut.tpid <= EVTOCD_TBL_FILTER_TPID_8100 ||
                        (fOut.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID &&
                        OMCI_GET_TPID_VALUE(inputTpid) == TPID_8100 &&
                        TPID_8100 == pOutVlan->tpid))
                    {
                        pVlanFilter->filterStagMode = VLAN_FILTER_VID;
                        pVlanFilter->filterSTag = tIn;
                    }
                    else
                    {
                        pVlanFilter->filterCtagMode = VLAN_FILTER_VID;
                        pVlanFilter->filterCTag = fOut;
                    }

                    pSVlanAct->vlanAct = VLAN_ACT_REMOVE;
                    pCVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
                    pOutVlan->outVlan = fOut;
                }
                //Insert 2 tags (X,Y), copy external and internal priority: S-C-F->Y-X-S-C-F
                else if (tOut.pri == EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_OUTER &&
                    tIn.pri == EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_INNER)
                {
                    pVlanFilter->filterStagMode = VLAN_FILTER_VID;
                    pVlanFilter->filterCtagMode = VLAN_FILTER_VID;
                    pVlanFilter->filterSTag = tOut;
                    pVlanFilter->filterCTag = tIn;
                    pSVlanAct->vlanAct = VLAN_ACT_REMOVE;
                    pCVlanAct->vlanAct = VLAN_ACT_REMOVE;
                    pOutVlan->outVlan = fOut;
                }
                //Insert 1 tag (X): S-C-F->X-S-C-F
                else if (tOut.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG)
                {
                    pVlanFilter->filterStagMode = VLAN_FILTER_VID | VLAN_FILTER_PRI;
                    pVlanFilter->filterCtagMode = VLAN_FILTER_VID;
                    pVlanFilter->filterSTag = tIn;
                    pVlanFilter->filterCTag = fOut;
                    pSVlanAct->vlanAct = VLAN_ACT_REMOVE;
                    pCVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
                    pOutVlan->outVlan = fOut;
                }
                //Insert 2 tags (X,Y): S-C-F->Y-X-S-C-F
                else
                {
                    pVlanFilter->filterStagMode = VLAN_FILTER_VID | VLAN_FILTER_PRI;
                    pVlanFilter->filterCtagMode = VLAN_FILTER_VID | VLAN_FILTER_PRI;
                    pVlanFilter->filterSTag = tOut;
                    pVlanFilter->filterCTag = tIn;
                    pSVlanAct->vlanAct = VLAN_ACT_REMOVE;
                    pCVlanAct->vlanAct = VLAN_ACT_REMOVE;
                    pOutVlan->outVlan = fOut;
                }
            }
            else if(1 == removeTag)
            {
                //Remove outer tag: S-C-F->C-F
                if (tOut.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG &&
                    tIn.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG)
                {
                    if (TPID_88A8 == pOutVlan->tpid || TPID_9100 == pOutVlan->tpid)
                    {
                        pVlanFilter->filterStagMode = VLAN_FILTER_VID;
                        pVlanFilter->filterCtagMode = VLAN_FILTER_NO_TAG;
                        pVlanFilter->filterSTag = fIn;
                    }
                    else if (TPID_8100 == pOutVlan->tpid)
                    {
                        pVlanFilter->filterStagMode = VLAN_FILTER_NO_TAG;
                        pVlanFilter->filterCtagMode = VLAN_FILTER_VID;
                        pVlanFilter->filterCTag = fIn;
                    }
                    if (fIn.tpid <= EVTOCD_TBL_FILTER_TPID_8100 ||
                        (fIn.tpid >=EVTOCD_TBL_FILTER_TPID_INPUT_TPID &&
                        OMCI_GET_TPID_VALUE(inputTpid) == TPID_8100))
                    {
                        pSVlanAct->vlanAct = VLAN_ACT_ADD;
                        pSVlanAct->priAct  = PRI_ACT_ASSIGN;
                        pSVlanAct->vidAct  = VID_ACT_ASSIGN;
                        pSVlanAct->assignVlan = fOut;
                        pCVlanAct->vlanAct = VLAN_ACT_NON;
                        pOutVlan->outVlan = fOut;
                    }
                    else if (fIn.tpid >= EVTOCD_TBL_FILTER_TPID_INPUT_TPID &&
                        (OMCI_GET_TPID_VALUE(inputTpid) == TPID_88A8 ||
                         OMCI_GET_TPID_VALUE(inputTpid) == TPID_9100))
                    {
                        pSVlanAct->vlanAct = VLAN_ACT_ADD;
                        pSVlanAct->priAct  = PRI_ACT_TRANSPARENT;
                        pSVlanAct->vidAct  = VID_ACT_ASSIGN;
                        pSVlanAct->assignVlan = fOut;
                        pCVlanAct->vlanAct = VLAN_ACT_NON;
                        pOutVlan->outVlan = pSVlanAct->assignVlan;
                    }
                }
                //Modify external tag, keep original priority: S-C-F->X-C-F
                else if (tOut.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG &&
                    tIn.pri == EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_OUTER)
                {
                    pVlanFilter->filterStagMode = VLAN_FILTER_VID;
                    pVlanFilter->filterCtagMode = VLAN_FILTER_VID;
                    pVlanFilter->filterSTag = tIn;
                    pVlanFilter->filterCTag = fIn;
                    pSVlanAct->vlanAct = VLAN_ACT_MODIFY;
                    pSVlanAct->priAct  = PRI_ACT_COPY_OUTER;
                    pSVlanAct->vidAct  = VID_ACT_ASSIGN;
                    pSVlanAct->assignVlan = fOut;
                    pCVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
                    pOutVlan->outVlan = fOut;
                }
                //Modify external tag: S-C-F->X-C-F
                else if (tOut.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG)
                {
                    pVlanFilter->filterStagMode = VLAN_FILTER_VID | VLAN_FILTER_PRI;
                    pVlanFilter->filterCtagMode = VLAN_FILTER_VID;
                    pVlanFilter->filterSTag = tIn;
                    pVlanFilter->filterCTag = fIn;
                    pSVlanAct->vlanAct = VLAN_ACT_MODIFY;
                    pSVlanAct->priAct  = PRI_ACT_TRANSPARENT;
                    pSVlanAct->vidAct  = VID_ACT_ASSIGN;
                    pSVlanAct->assignVlan = fOut;
                    pCVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
                    pOutVlan->outVlan = fOut;
                }
            }
            else if(2 == removeTag)
            {
                //Remove both tags: S-C-F->F
                if (tOut.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG &&
                    tIn.pri == EVTOCD_TBL_TREATMENT_PRI_DO_NOT_ADD_TAG)
                {
                    pVlanFilter->filterStagMode = VLAN_FILTER_NO_TAG;
                    pVlanFilter->filterCtagMode = VLAN_FILTER_NO_TAG;
                    pSVlanAct->vlanAct = VLAN_ACT_ADD;
                    pSVlanAct->priAct  = PRI_ACT_ASSIGN;
                    pSVlanAct->vidAct  = VID_ACT_ASSIGN;
                    pSVlanAct->assignVlan = fOut;
                    pCVlanAct->vlanAct = VLAN_ACT_ADD;
                    pCVlanAct->priAct  = PRI_ACT_ASSIGN;
                    pCVlanAct->vidAct  = VID_ACT_ASSIGN;
                    pCVlanAct->assignVlan = fIn;
                    pOutVlan->outVlan = fOut;
                }
                //Modify both tags,keep original priorities: S-C-F->Y-X-F
                else if (tOut.pri == EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_OUTER &&
                    tIn.pri == EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_INNER)
                {
                    pVlanFilter->filterStagMode = VLAN_FILTER_VID;
                    pVlanFilter->filterCtagMode = VLAN_FILTER_VID;
                    pVlanFilter->filterSTag = tOut;
                    pVlanFilter->filterCTag = tIn;
                    pSVlanAct->vlanAct = VLAN_ACT_MODIFY;
                    pSVlanAct->priAct  = PRI_ACT_COPY_OUTER;
                    pSVlanAct->vidAct  = VID_ACT_ASSIGN;
                    pSVlanAct->assignVlan = fOut;
                    pCVlanAct->vlanAct = VLAN_ACT_MODIFY;
                    pCVlanAct->priAct  = PRI_ACT_COPY_INNER;
                    pCVlanAct->vidAct  = VID_ACT_ASSIGN;
                    pCVlanAct->assignVlan = fIn;
                    pOutVlan->outVlan = fOut;
                }
                //Swap both tags: S-C-F->C-S-F
                else if (tOut.pri == EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_INNER &&
                    tIn.pri == EVTOCD_TBL_TREATMENT_PRI_COPY_FROM_OUTER &&
                    tOut.vid == EVTOCD_TBL_TREATMENT_VID_COPY_FROM_INNER &&
                    tIn.vid == EVTOCD_TBL_TREATMENT_VID_COPY_FROM_OUTER)
                {
                    pVlanFilter->filterStagMode = VLAN_FILTER_VID | VLAN_FILTER_PRI;
                    pVlanFilter->filterCtagMode = VLAN_FILTER_VID | VLAN_FILTER_PRI;
                    pVlanFilter->filterSTag = fIn;
                    pVlanFilter->filterCTag = fOut;
                    pSVlanAct->vlanAct = VLAN_ACT_MODIFY;
                    pSVlanAct->priAct  = PRI_ACT_COPY_INNER;
                    pSVlanAct->vidAct  = VID_ACT_COPY_INNER;
                    pSVlanAct->assignVlan = fOut;
                    pCVlanAct->vlanAct = VLAN_ACT_MODIFY;
                    pCVlanAct->priAct  = PRI_ACT_COPY_OUTER;
                    pCVlanAct->vidAct  = VID_ACT_COPY_OUTER;
                    pCVlanAct->assignVlan = fIn;
                    pOutVlan->outVlan = fIn;
                }
                //Modify both tags: S-C-F->Y-X-F
                else
                {
                    pVlanFilter->filterStagMode = VLAN_FILTER_VID | VLAN_FILTER_PRI;
                    pVlanFilter->filterCtagMode = VLAN_FILTER_VID | VLAN_FILTER_PRI;
                    pVlanFilter->filterSTag = tOut;
                    pVlanFilter->filterCTag = tIn;
                    pSVlanAct->vlanAct = VLAN_ACT_MODIFY;
                    pSVlanAct->priAct  = PRI_ACT_ASSIGN;
                    pSVlanAct->vidAct  = VID_ACT_ASSIGN;
                    pSVlanAct->assignVlan = fOut;
                    pCVlanAct->vlanAct = VLAN_ACT_MODIFY;
                    pCVlanAct->priAct  = PRI_ACT_ASSIGN;
                    pCVlanAct->vidAct  = VID_ACT_ASSIGN;
                    pCVlanAct->assignVlan = fIn;
                    pOutVlan->outVlan = fOut;
                }
            }
        }

    }

    omci_vlan_xslateEthType(&pVlanFilter->etherType, pVlanFilter->etherType);

    return GOS_OK;
}


static GOS_ERROR_CODE
omci_checkIgnoreExtVlan(MIB_TREE_CONN_T *pConn, BOOL *pVal)
{
    MIB_TABLE_MCASTSUBCONFINFO_T        subConInfo, *pMibSubConInfo = NULL;
    mopTableEntry_t                     *pEntry = NULL;
    MIB_TABLE_MCASTOPERPROF_T           mop, *pMibMop = NULL;

    OMCI_ERR_CHK(OMCI_LOG_LEVEL_ERR, (!pVal), GOS_FAIL);

    memset(&subConInfo, 0, sizeof(MIB_TABLE_MCASTSUBCONFINFO_T));
    subConInfo.EntityId = pConn->pUniPort->EntityID;
    if (FALSE == mib_FindEntry(MIB_TABLE_MCASTSUBCONFINFO_INDEX, &subConInfo, &pMibSubConInfo))
    {
        return GOS_FAIL;
    }

    LIST_FOREACH(pEntry, &pMibSubConInfo->MOPhead, entries)
    {
        memset(&mop, 0, sizeof(MIB_TABLE_MCASTOPERPROF_T));
        mop.EntityId = pEntry->tableEntry.mcastOperProfPtr;
        if (TRUE == mib_FindEntry(MIB_TABLE_MCASTOPERPROF_INDEX, &mop, &pMibMop))
        {
            feature_api(FEATURE_API_MC_00000001, pMibMop->DownstreamIgmpMulticastTci);

            if (PASS != pMibMop->DownstreamIgmpMulticastTci[0])
            {
                *pVal = TRUE;

                return GOS_OK;
            }
        }
    }
    return GOS_FAIL;
}

static GOS_ERROR_CODE
omci_checkVlan(MIB_TREE_CONN_T *pConn, omci_vlan_rule_t *pTrafficRule)
{
    int i = 0, arraySize = 0;
    UINT16 vid = 0xffff;
    UINT8 pri = 0xff;
    MIB_TABLE_VLANTAGFILTERDATA_T *pAniVlanFilter, *pUniVlanFilter;
    UINT8  aniFwdOp = VTFD_FWD_OP_TAG_A_UNTAG_A, uniFwdOp = VTFD_FWD_OP_TAG_A_UNTAG_A; /*default set to forward all(don't care vid and pri)*/
    UINT16 tciTbl[MIB_TABLE_VLANTAGFILTERDATA_FILTERTBL_LEN/2];
    BOOL    ignoreB = FALSE;
    MIB_TABLE_GEMPORTCTP_T  mibGpnc;

    pAniVlanFilter = pConn->pAniVlanTagFilter;
    pUniVlanFilter = pConn->pUniVlanTagFilter;

    if((pAniVlanFilter == NULL) && (pUniVlanFilter == NULL))
    {
        /* unmatch */
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s(), %d, NO ani vlan tag filter pointer", __FUNCTION__, __LINE__);
        return GOS_OK;
    }

    if(pAniVlanFilter != NULL)
        aniFwdOp = pAniVlanFilter->FwdOp;
    if(pUniVlanFilter != NULL && VTFD_FWD_OP_TAG_A_UNTAG_A != pUniVlanFilter->FwdOp)
        uniFwdOp = pUniVlanFilter->FwdOp;

    omci_GenVlanFilterVlanTbl(pConn, tciTbl, &arraySize);

    if (PON_GEMPORT_DIRECTION_DS == pTrafficRule->dir && pConn->pMcastGemIwTp)
    {
        mibGpnc.EntityID = pConn->pMcastGemIwTp->GemCtpPtr;
        if (GOS_OK == MIB_Get(MIB_TABLE_GEMPORTCTP_INDEX, &mibGpnc, sizeof(MIB_TABLE_GEMPORTCTP_T)))
        {
            if (mibGpnc.PortID == pTrafficRule->outgress)
            {
                omci_checkIgnoreExtVlan(pConn, &ignoreB);
            }
        }
    }

    if (CARE_VID(aniFwdOp) || CARE_VID(uniFwdOp))
    {
        for(i=0; i < arraySize; i++)
        {
            vid = tciTbl[i] & 0xfff;
            /* match */
            if (PON_GEMPORT_DIRECTION_DS == pTrafficRule->dir)
            {
                if (VLAN_FILTER_VID & pTrafficRule->vlanRule.filterRule.filterStagMode ||
                    VLAN_FILTER_TCI & pTrafficRule->vlanRule.filterRule.filterStagMode)
                {
                    if (ignoreB)
                    {
                        return GOS_ERR_DISABLE;
                    }
                    if (vid == pTrafficRule->vlanRule.filterRule.filterSTag.vid)
                        return GOS_OK;
                }
                else if (VLAN_FILTER_VID & pTrafficRule->vlanRule.filterRule.filterCtagMode ||
                    VLAN_FILTER_TCI & pTrafficRule->vlanRule.filterRule.filterCtagMode)
                {
                    if (ignoreB)
                    {
                        return GOS_ERR_DISABLE;
                    }
                    if (vid == pTrafficRule->vlanRule.filterRule.filterCTag.vid)
                        return GOS_OK;
                }
            }
            else
            {
                if (FAL_OK == feature_api(FEATURE_API_BDP_00000001, &vid, pTrafficRule))
                    return GOS_OK;
                else
                {
                    if (vid == pTrafficRule->vlanRule.outStyle.outVlan.vid)
                        return GOS_OK;
                    else if (4096 == pTrafficRule->vlanRule.outStyle.outVlan.vid)
                        return GOS_ERR_DISABLE;
                }
            }
        }
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s(), %d,  ani vlan tag filter pointer, vid not match", __FUNCTION__, __LINE__);
        return GOS_FAIL;
    }
    else if (CARE_PRI(aniFwdOp) || CARE_PRI(uniFwdOp))
    {
        for(i=0; i < arraySize; i++)
        {
            pri = (tciTbl[i] >> 13) & 0xF;
            /* match */
            if (PON_GEMPORT_DIRECTION_DS == pTrafficRule->dir)
            {
                if (VLAN_FILTER_PRI & pTrafficRule->vlanRule.filterRule.filterStagMode ||
                    VLAN_FILTER_TCI & pTrafficRule->vlanRule.filterRule.filterStagMode)
                {
                    if (ignoreB)
                    {
                        return GOS_ERR_DISABLE;
                    }
                    if (pri == pTrafficRule->vlanRule.filterRule.filterSTag.pri)
                        return GOS_OK;
                }
                else if (VLAN_FILTER_PRI & pTrafficRule->vlanRule.filterRule.filterCtagMode ||
                    VLAN_FILTER_TCI & pTrafficRule->vlanRule.filterRule.filterCtagMode)
                {
                    if (ignoreB)
                    {
                        return GOS_ERR_DISABLE;
                    }
                    if (pri == pTrafficRule->vlanRule.filterRule.filterCTag.pri)
                        return GOS_OK;
                }
            }
            else
            {
                if (pri == pTrafficRule->vlanRule.outStyle.outVlan.pri)
                    return GOS_OK;
            }
        }
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s(), %d,  ani vlan tag filter pointer, pri not match", __FUNCTION__, __LINE__);
        return GOS_FAIL;
    }
    else if (VTFD_FWD_OP_TAG_C_UNTAG_A == aniFwdOp)
    {
        if (PON_GEMPORT_DIRECTION_DS == pTrafficRule->dir)
        {
            // TBD
            if (ignoreB)
                return GOS_ERR_DISABLE;
        }
        else
        {
            if(pTrafficRule->vlanRule.outStyle.outTagNum != 0)
                return GOS_FAIL;
        }
    }
    else
    {
        return GOS_OK;
    }

    return GOS_OK;
}

static BOOL omci_check_treat_outer_pri_to_filter_outer_pri
    (MIB_TREE_CONN_T *pConn, OMCI_VLAN_ACT_ts vlanAct)
{
    int i, max = 8;
    if ((VLAN_ACT_ADD == vlanAct.vlanAct ||
        VLAN_ACT_MODIFY == vlanAct.vlanAct) &&
        PRI_ACT_ASSIGN == vlanAct.priAct)
    {
        //TBD: PRI_ACT_FROM_DSCP
        for(i = 0; i < max; i++)
        {
            if(pConn->pGemPortCtp[i] &&
                i == vlanAct.assignVlan.pri)
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}

static void omci_changeOuterTagFilterMode(MIB_TREE_CONN_T *pConn,int index,omci_vlan_rule_t *pTrafficRule)
{
    if(OMCI_TRAF_MODE_8021P_BASE == pConn->traffMode &&
        OMCI_PRI_FILTER_IGNORE == pTrafficRule->vlanRule.filterRule.filterSTag.pri &&
        (omci_check_treat_outer_pri_to_filter_outer_pri(pConn, pTrafficRule->vlanRule.sTagAct)))
    {
        pTrafficRule->vlanRule.filterRule.filterStagMode |= VLAN_FILTER_PRI;
        pTrafficRule->vlanRule.filterRule.filterSTag.pri = index;
    }
    else if(OMCI_TRAF_MODE_8021P_BASE == pConn->traffMode &&
            VLAN_FILTER_NO_TAG == pTrafficRule->vlanRule.filterRule.filterStagMode &&
            OMCI_PRI_FILTER_IGNORE == pTrafficRule->vlanRule.filterRule.filterCTag.pri &&
            (omci_check_treat_outer_pri_to_filter_outer_pri(pConn, pTrafficRule->vlanRule.cTagAct)) &&
            (omci_check_treat_outer_pri_to_filter_outer_pri(pConn, pTrafficRule->vlanRule.sTagAct)))
    {
        pTrafficRule->vlanRule.filterRule.filterCtagMode |= VLAN_FILTER_PRI;
        pTrafficRule->vlanRule.filterRule.filterCTag.pri = index;
    }
    /*For treatment Pbit form DSCP*/
    /*Have Stag*/
    else if(OMCI_TRAF_MODE_8021P_BASE == pConn->traffMode &&
        OMCI_PRI_FILTER_IGNORE == pTrafficRule->vlanRule.filterRule.filterSTag.pri &&
        PRI_ACT_FROM_DSCP == pTrafficRule->vlanRule.sTagAct.priAct)
    {
        pTrafficRule->vlanRule.filterRule.filterStagMode |= VLAN_FILTER_DSCP_PRI;
        pTrafficRule->vlanRule.filterRule.filterSTag.pri = index;
    }
    /*For Ctag only and Untag to Ctag*/
    else if(OMCI_TRAF_MODE_8021P_BASE == pConn->traffMode &&
            VLAN_FILTER_NO_TAG == pTrafficRule->vlanRule.filterRule.filterStagMode &&
            ((OMCI_PRI_FILTER_IGNORE == pTrafficRule->vlanRule.filterRule.filterCTag.pri &&
             PRI_ACT_FROM_DSCP == pTrafficRule->vlanRule.cTagAct.priAct)
            || ((VLAN_FILTER_NO_TAG & pTrafficRule->vlanRule.filterRule.filterCtagMode) &&
                PRI_ACT_FROM_DSCP == pTrafficRule->vlanRule.cTagAct.priAct)))
    {
        pTrafficRule->vlanRule.filterRule.filterCtagMode |= VLAN_FILTER_DSCP_PRI;
        pTrafficRule->vlanRule.filterRule.filterCTag.pri = index;
    }
    return;
}

// if no vlan filtering list should be returned and configure original omci vlan rule
static GOS_ERROR_CODE
omci_MergeExtRuleWithVlanlist(MIB_TREE_CONN_T *pConn, int index, omci_vlan_rule_t *pTemplate)
{
    GOS_ERROR_CODE ret;
    omci_vlan_rule_t *pNew = NULL;
    int i = 0, arraySize = 0;
    UINT16 vid = 0xffff;
    UINT8 pri = 0xff;
    MIB_TABLE_VLANTAGFILTERDATA_T *pAniVlanFilter, *pUniVlanFilter;
    UINT8   aniFwdOp = VTFD_FWD_OP_TAG_A_UNTAG_A, uniFwdOp = VTFD_FWD_OP_TAG_A_UNTAG_A;
    BOOL    filterVidB = FALSE, filterPriB = FALSE;
    UINT16 tciTbl[MIB_TABLE_VLANTAGFILTERDATA_FILTERTBL_LEN/2];

    pAniVlanFilter = pConn->pAniVlanTagFilter;
    pUniVlanFilter = pConn->pUniVlanTagFilter;

    // handle original omci vlan rule
    if (!pAniVlanFilter && !pUniVlanFilter)
        return GOS_OK;

    if(pAniVlanFilter != NULL)
        aniFwdOp = pAniVlanFilter->FwdOp;
    if(pUniVlanFilter != NULL)
        uniFwdOp = pUniVlanFilter->FwdOp;

    omci_GenVlanFilterVlanTbl(pConn, tciTbl, &arraySize);

    // cannot configure original vlan rule if default untag rule created by OLT and drop untag forward operation
    if (VLAN_FILTER_NO_TAG & pTemplate->vlanRule.filterRule.filterStagMode &&
        VLAN_FILTER_NO_TAG & pTemplate->vlanRule.filterRule.filterCtagMode &&
        (DROP_UNTAG(aniFwdOp) || DROP_UNTAG(uniFwdOp)))
        return GOS_ERR_DISABLE;

    // handle configure original omci vlan rule
    if (arraySize == 0)
        return GOS_OK;

    if (CARE_VID(aniFwdOp) || CARE_VID(uniFwdOp)) filterVidB = TRUE;

    if (CARE_PRI(aniFwdOp) || CARE_PRI(uniFwdOp)) filterPriB = TRUE;

    // handle configure original omci vlan rule
    if (!filterVidB && !filterPriB)
        return GOS_OK;

    for (i = 0; i < arraySize; i++)
    {
        if (filterVidB)
            vid = tciTbl[i] & 0xfff;
        if (filterPriB)
            pri = (tciTbl[i] >> 13) & 0xF;

        if (NULL != (pNew = (omci_vlan_rule_t *)malloc(sizeof(omci_vlan_rule_t))))
        {
            memset(pNew, 0, sizeof(omci_vlan_rule_t));
            memcpy(pNew, pTemplate, sizeof(omci_vlan_rule_t));
            if (VLAN_FILTER_VID & pNew->vlanRule.filterRule.filterStagMode)
            {
                if (filterVidB)
                    pNew->vlanRule.filterRule.filterSTag.vid = vid;
            }
            else if (VLAN_FILTER_PRI & pNew->vlanRule.filterRule.filterStagMode)
            {
                if (filterPriB)
                    pNew->vlanRule.filterRule.filterSTag.pri = pri;
            }
            else if (VLAN_FILTER_TCI & pNew->vlanRule.filterRule.filterStagMode)
            {
                if (filterVidB)
                    pNew->vlanRule.filterRule.filterSTag.vid = vid;
                if (filterPriB)
                    pNew->vlanRule.filterRule.filterSTag.pri = pri;
            }
            else if (VLAN_FILTER_CARE_TAG & pNew->vlanRule.filterRule.filterStagMode)
            {
                if (filterVidB)
                {
                    pNew->vlanRule.filterRule.filterStagMode |= VLAN_FILTER_VID;
                    pNew->vlanRule.filterRule.filterSTag.vid = vid;
                }
                if (filterPriB)
                {
                    pNew->vlanRule.filterRule.filterStagMode |= VLAN_FILTER_PRI;
                    pNew->vlanRule.filterRule.filterSTag.pri = pri;
                }
            }
            else if (VLAN_FILTER_VID & pNew->vlanRule.filterRule.filterCtagMode)
            {
                if (filterVidB)
                    pNew->vlanRule.filterRule.filterCTag.vid = vid;

            }
            else if (VLAN_FILTER_PRI & pNew->vlanRule.filterRule.filterCtagMode)
            {
                if (filterPriB)
                    pNew->vlanRule.filterRule.filterCTag.pri = pri;
            }
            else if (VLAN_FILTER_TCI & pNew->vlanRule.filterRule.filterCtagMode)
            {
                if (filterVidB)
                    pNew->vlanRule.filterRule.filterCTag.vid = vid;
                if (filterPriB)
                    pNew->vlanRule.filterRule.filterCTag.pri = pri;
            }
            else if (VLAN_FILTER_CARE_TAG & pNew->vlanRule.filterRule.filterCtagMode)
            {
                if (filterVidB)
                {
                    pNew->vlanRule.filterRule.filterCtagMode |= VLAN_FILTER_VID;
                    pNew->vlanRule.filterRule.filterCTag.vid = vid;
                }
                if (filterPriB)
                {
                    pNew->vlanRule.filterRule.filterCtagMode |= VLAN_FILTER_PRI;
                    pNew->vlanRule.filterRule.filterCTag.pri = pri;
                }
            }

            ret = (((pNew->dir == PON_GEMPORT_DIRECTION_DS) &&
                    (NULL != pConn->pMcastGemIwTp) &&
                    (GOS_OK != omci_McTransparentFwd(pConn, pNew))) ?
                    omci_AddMcastTrafficRule(pConn, index, pNew) : omci_AddTrafficRule(pConn, index, pNew));
            if (GOS_FAIL == ret) free(pNew);
        }
    }

    return GOS_ERR_DISABLE;
}

static GOS_ERROR_CODE
omci_expand_rule_by_internal_pri(MIB_TREE_CONN_T    *pConn, int index,
                                 omci_vlan_rule_t   *pRule, unsigned char bitmap)
{
    omci_vlan_rule_t    *pNewRule = NULL;
    unsigned char       priority_idx;
    unsigned char start_idx,end_idx;


    if (!pRule || 0 == bitmap)
        return GOS_ERR_INVALID_INPUT;

    if(OMCI_TRAF_MODE_FLOW_BASE == pConn->traffMode)
    {
        start_idx = 0;
        end_idx = 7;
    }
    else{/*8021P base,only apply specified priority*/
        start_idx = end_idx = index;
    }


    for (priority_idx = start_idx; priority_idx <= end_idx; priority_idx++)
    {
        if (0 == ((bitmap >> priority_idx) & 0x1))
            continue;

        // check priority_idx is used or not and update usage
        if (!omci_GetNonUsedPriority(priority_idx))
            continue;

        if (NULL != (pNewRule = (omci_vlan_rule_t*)malloc(sizeof(omci_vlan_rule_t))))
        {
            memcpy(pNewRule, pRule, sizeof(omci_vlan_rule_t));

            if (PRI_ACT_FROM_DSCP == pNewRule->vlanRule.sTagAct.priAct)
            {
                pNewRule->vlanRule.sTagAct.assignVlan.pri = priority_idx;
                pNewRule->vlanRule.outStyle.outVlan.pri = priority_idx;
            }
            if (PRI_ACT_FROM_DSCP == pNewRule->vlanRule.cTagAct.priAct)
            {
                pNewRule->vlanRule.cTagAct.assignVlan.pri = priority_idx;
                pNewRule->vlanRule.outStyle.outVlan.pri = priority_idx;
            }

            if (GOS_FAIL == omci_AddTrafficRule(pConn, index, pNewRule))
                free(pNewRule);
        }
        break;
    }
    return GOS_OK;
}

/* original VLAN filter info is equal to outer treatment of Ext. VLAN */
static GOS_ERROR_CODE
omci_MergeVlanTagFilterRule(MIB_TREE_CONN_T *pConn,int index,omci_vlan_rule_t *pTrafficRule)
{
    GOS_ERROR_CODE ret = GOS_OK;
    GOS_ERROR_CODE vlan_ck_ret = GOS_OK;
    OMCI_VLAN_OPER_ts  *pVlanOper = &pTrafficRule->vlanRule;
    OMCI_VLAN_OUT_ts   *pVlanOut  = &pVlanOper->outStyle;
    omci_vlan_rule_t *pTmpRule = NULL;
    unsigned char   pbitBitmap = 0;

    if(OMCI_EXTVLAN_REMOVE_TAG_DISCARD == pVlanOut->isDefaultRule)
        pTrafficRule->dir = PON_GEMPORT_DIRECTION_US;

    if(pConn->traffMode == OMCI_TRAF_MODE_8021P_BASE && pVlanOut->outVlan.pri < 8 && pVlanOut->outVlan.pri !=index) return GOS_ERR_STATE;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "cMode=%u, sMode=%u ", pTrafficRule->vlanRule.filterRule.filterCtagMode, pTrafficRule->vlanRule.filterRule.filterStagMode);

    /* for default transparent rule, add vlan rule by merging with VLAN filter list*/
    if (1 == pTrafficRule->vlanRule.outStyle.isDefaultRule &&
        PON_GEMPORT_DIRECTION_DS != pTrafficRule->dir &&
        GOS_ERR_DISABLE == omci_MergeExtRuleWithVlanlist(pConn, index, pTrafficRule))
    {
        free(pTrafficRule);
        return GOS_OK;
    }

    /* check vlan filter element should be within treatment of ext vlan */
    if (GOS_FAIL == (vlan_ck_ret = omci_checkVlan(pConn, pTrafficRule))) return GOS_ERR_STATE;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s()@%d, vlan_ck_ret=%u", __FUNCTION__, __LINE__, vlan_ck_ret);

    if (GOS_ERR_DISABLE == vlan_ck_ret &&
        /*PON_GEMPORT_DIRECTION_DS == pTrafficRule->dir &&*/
        GOS_ERR_DISABLE == omci_MergeExtRuleWithVlanlist(pConn, index, pTrafficRule))
    {
        free(pTrafficRule);
        return GOS_OK;
    }

    /* change outer tag filter mode to include filter PBIT if 8021p mapper only map the specefic gem port  */
    omci_changeOuterTagFilterMode(pConn, index, pTrafficRule);

    if(ETHTYPE_FILTER_PPPOE == pTrafficRule->vlanRule.filterRule.etherType)
    {
        pTmpRule = (omci_vlan_rule_t*)malloc(sizeof(omci_vlan_rule_t));
        if(NULL != pTmpRule)
        {
            memcpy(pTmpRule, pTrafficRule, sizeof(omci_vlan_rule_t));
            pTmpRule->vlanRule.filterRule.etherType = ETHTYPE_FILTER_PPPOE_S;
            if (GOS_FAIL == omci_AddTrafficRule(pConn, index, pTmpRule))
                free(pTmpRule);
        }
    }

    if (PON_GEMPORT_DIRECTION_DS != pTrafficRule->dir &&
        (PRI_ACT_FROM_DSCP == pTrafficRule->vlanRule.sTagAct.priAct ||
         PRI_ACT_FROM_DSCP == pTrafficRule->vlanRule.cTagAct.priAct))
    {
        //expand rule according to internal priority comes from dscp2pbit table
        if (GOS_OK  == omci_GetDscp2pbitBitmap(&pbitBitmap))
        {
            //pbit has been translated and OLT maybe filter priority for incomming packet.
            //Therefore, internal priority value is put in assignVlan of OMCI_VLAN_ACT_ts
            if (GOS_OK == omci_expand_rule_by_internal_pri(
                            pConn, index, pTrafficRule, pbitBitmap))
            {
                free(pTrafficRule);
                return GOS_OK;
            }
        }
    }

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "index %d, etype=%u, ret=%u",
            index, pTrafficRule->vlanRule.filterRule.etherType, ret);

    ret = (((pTrafficRule->dir == PON_GEMPORT_DIRECTION_DS) &&
            (NULL != pConn->pMcastGemIwTp) &&
            (GOS_OK != omci_McTransparentFwd(pConn, pTrafficRule))) ?
            omci_AddMcastTrafficRule(pConn, index, pTrafficRule) : omci_AddTrafficRule(pConn, index, pTrafficRule));

    if(ret == GOS_FAIL)
        return GOS_ERR_STATE;

    return GOS_OK;
}

static GOS_ERROR_CODE
omci_McTransparentFwd(MIB_TREE_CONN_T *pConn, omci_vlan_rule_t *pTrafficRule)
{
    MIB_TABLE_MCASTSUBCONFINFO_T subConInfo, *pMibSubConInfo = NULL;

    if (!pConn->pUniPort)
        return GOS_OK;

    memset(&subConInfo, 0, sizeof(MIB_TABLE_MCASTSUBCONFINFO_T));

    subConInfo.EntityId = pConn->pUniPort->EntityID;

    if (FALSE == mib_FindEntry(MIB_TABLE_MCASTSUBCONFINFO_INDEX, &subConInfo, &pMibSubConInfo))
    {
        return GOS_OK;
    }
    return GOS_FAIL;
}

static GOS_ERROR_CODE
omci_GenExtVlanSingleRule(MIB_TREE_CONN_T *pConn, int index,extvlanTableEntry_t *pEntry)
{
    omci_vlan_rule_t *pRule;
    OMCI_VLAN_OPER_ts *pVlanRule;
    omci_extvlan_row_entry_t *pExtVlan = &pEntry->tableEntry;
    OMCI_VLAN_ts fInnerVlan,fOuterVlan,tInnerVlan,tOuterVlan;
    OMCI_VLAN_FILTER_ts *pVlanFilter;
    unsigned int removeTag;
    MIB_TABLE_EXTVLANTAGOPERCFGDATA_T *pExtVlanCfg = (MIB_TABLE_EXTVLANTAGOPERCFGDATA_T*)pConn->pUniExtVlanCfg;
    int ingress;
    OMCI_VLAN_FILTER_MODE_e filterCtagMode, filterStagMode;

    ingress = omci_GetIngressByUniMbpcd(pConn);

    omci_CreateTraffRule(pConn, index, &pRule, ingress);
    pVlanRule = &pRule->vlanRule;
    pVlanRule->filterMode = VLAN_OPER_MODE_EXTVLAN;
    pVlanFilter = &pVlanRule->filterRule;
    /*assign VlanFilterRule*/
    fOuterVlan.pri = pExtVlan->outerFilterWord.bit.filterOuterPri;
    fOuterVlan.vid = pExtVlan->outerFilterWord.bit.filterOuterVid;
    fOuterVlan.tpid =pExtVlan->outerFilterWord.bit.filterOuterTpId;
    /*filter inner tag*/
    fInnerVlan.pri = pExtVlan->innerFilterWord.bit.filterInnerPri;
    fInnerVlan.vid = pExtVlan->innerFilterWord.bit.filterInnerVid;
    fInnerVlan.tpid =pExtVlan->innerFilterWord.bit.filterInnerTpId;
    pVlanFilter->etherType = pExtVlan->innerFilterWord.bit.filterEthType;
    /*vlan act*/
    removeTag = pExtVlan->outerTreatmentWord.bit.treatment;
    tOuterVlan.pri  = pExtVlan->outerTreatmentWord.bit.treatmentOuterPri;
    tOuterVlan.vid  = pExtVlan->outerTreatmentWord.bit.treatmentOuterVid;
    tOuterVlan.tpid = pExtVlan->outerTreatmentWord.bit.treatmentOuterTpId;
    /*inner act*/
    tInnerVlan.pri  = pExtVlan->innerTreatmentWord.bit.treatmentInnerPri;
    tInnerVlan.vid  = pExtVlan->innerTreatmentWord.bit.treatmentInnerVid;
    tInnerVlan.tpid = pExtVlan->innerTreatmentWord.bit.treatmentInnerTpId;

    if (PON_GEMPORT_DIRECTION_DS == pRule->dir &&
        ((OMCI_ETHTYPE_FILTER_MODE_e)EVTOCD_TBL_FILTER_ET_IPOE_0800 < pVlanFilter->etherType ||
        EVTOCD_TBL_TREATMENT_DISCARD_FRAME == removeTag))
    {
        /* ignore downstream multicast rule with ether type (exclude IPOE) since classify rule is not enough */
        free(pRule);
        return GOS_OK;
    }

    if (PON_GEMPORT_DIRECTION_DS == pRule->dir)
        omci_GetExtVlanRuleByDsDir(pExtVlanCfg->InputTPID, pExtVlanCfg->OutputTPID,
            fOuterVlan,fInnerVlan,tOuterVlan,tInnerVlan,removeTag,&pRule->vlanRule);
    else
        omci_GetExtVlanRule(pExtVlanCfg->InputTPID, pExtVlanCfg->OutputTPID,
            fOuterVlan,fInnerVlan,tOuterVlan,tInnerVlan,removeTag,&pRule->vlanRule);

    /*Don't set CF rule to drop when filter are S or C no care.
      Use last default DS CF action to drop DS and use unmatch CF action to drop US.
      For avoid this CF rule of drop action is set before other CF rules*/
    filterCtagMode = pRule->vlanRule.filterRule.filterCtagMode;
    filterStagMode = pRule->vlanRule.filterRule.filterStagMode;
    if(pRule->vlanRule.outStyle.isDefaultRule == OMCI_EXTVLAN_REMOVE_TAG_DISCARD &&
        ((filterCtagMode == VLAN_FILTER_CARE_TAG && filterStagMode == VLAN_FILTER_CARE_TAG)
           || (filterCtagMode == VLAN_FILTER_CARE_TAG && filterStagMode == VLAN_FILTER_NO_TAG)
           || (filterCtagMode == VLAN_FILTER_NO_TAG && filterStagMode == VLAN_FILTER_CARE_TAG)))
    {
        free(pRule);

        if(FAL_OK == feature_api(FEATURE_API_BDP_00000080, pConn,index))
        {
            return GOS_OK;
        }
        // return fail for default drop rule
        // so we won't create conn from ani vtfd
        return GOS_ERR_STATE;
    }

    pVlanRule->outStyle.dsMode = pExtVlanCfg->DsMode;

    if (GOS_ERR_STATE == omci_MergeVlanTagFilterRule(pConn,index,pRule))
    {
        free(pRule);
        return GOS_ERR_STATE;
    }
    return GOS_OK;
}

static BOOL omci_check_dscp2pbit_by_index(UINT8 *p, UINT8 index)
{
    UINT32 i, j;
    UINT32 tempPbit;

    for(i = 0; i < MIB_TABLE_DSCPTOPBITMAPPING_LEN; i += 3)
    {
        tempPbit = (p[i] << 16) + (p[i + 1] << 8)
                   + (p[i + 2]);
        for(j = 0; j < 8 ; j++)
        {
            if (index == ((tempPbit & 0xE00000) >> 21))
                return TRUE;
            tempPbit <<= 3;
        }
    }
    return FALSE;
}

static GOS_ERROR_CODE omci_GenNoTagWithDscpFilterRulePerIngress(
    MIB_TREE_CONN_T *pConn, int index, BOOL flag, int ingress)
{
    GOS_ERROR_CODE ret              = GOS_OK;
    omci_vlan_rule_t *pRule         = NULL;
    OMCI_VLAN_OPER_ts *pVlanRule    = NULL;

    if (MAP_8021P_UNMARKED_OPT_DSCP21P == pConn->p8021Map->UnmarkFrmOpt)
    {
        if (!omci_check_dscp2pbit_by_index(pConn->p8021Map->DscpMap2Pbit, (UINT8)index))
            return ret;
    }
    else /*default pbit */
    {
        if (pConn->p8021Map->DefPbitMark != index)
            return ret;
    }
    omci_CreateTraffRule(pConn, index, &pRule, ingress);

    pVlanRule = &pRule->vlanRule;

    if (MAP_8021P_UNMARKED_OPT_DSCP21P == pConn->p8021Map->UnmarkFrmOpt)
    {
        pVlanRule->filterMode = VLAN_OPER_MODE_FILTER_INNER_PRI;
        pVlanRule->filterRule.filterCtagMode |= (VLAN_FILTER_DSCP_PRI | VLAN_FILTER_NO_TAG);
        pVlanRule->filterRule.filterCTag.pri = index;
        pVlanRule->filterRule.filterCTag.vid = OMCI_VID_FILTER_IGNORE;
    }
    else
    {
        pVlanRule->filterMode = VLAN_OPER_MODE_FORWARD_UNTAG;
    }

    if (GOS_OK != omci_UpdateTrafficRuleByManual(pConn, index, pRule))
        goto fail_free;

    if (GOS_FAIL == feature_api(FEATURE_API_BDP_00000040, pRule))
        goto fail_free;

    if (NULL == pConn->pUniExtVlanCfg || TRUE == flag)
    {
        ret = (((pRule->dir == PON_GEMPORT_DIRECTION_DS) &&
                (NULL != pConn->pMcastGemIwTp) &&
                (GOS_OK != omci_McTransparentFwd(pConn, pRule))) ?
                omci_AddMcastTrafficRule(pConn, index, pRule) : omci_AddTrafficRule(pConn, index, pRule));

        if (GOS_OK == ret)
            return ret;
    }

fail_free:
    if(pRule)
        free(pRule);
    return ret;

}

static GOS_ERROR_CODE omci_GenNoVlanFilterRulePerIngress(
    MIB_TREE_CONN_T *pConn, int index, BOOL flag, int ingress)
{
    GOS_ERROR_CODE ret = GOS_OK;
    omci_vlan_rule_t *pRule;
    OMCI_VLAN_OPER_ts *pVlanRule;
    OMCI_VLAN_OUT_ts *pOutVlan;

    omci_CreateTraffRule(pConn, index, &pRule, ingress);
    pVlanRule = &pRule->vlanRule;
    pOutVlan = &pVlanRule->outStyle;
    /*All PASS*/
    if(pConn->traffMode == OMCI_TRAF_MODE_FLOW_BASE)
    {
        pVlanRule->filterMode = (NULL == pConn->pAniVlanTagOpCfg ?
            VLAN_OPER_MODE_FORWARD_ALL :  VLAN_OPER_MODE_VLANTAG_OPER);
    }else
    /*1:M*/
    {
        pVlanRule->filterMode = (NULL == pConn->pAniVlanTagOpCfg ?
            VLAN_OPER_MODE_FILTER_INNER_PRI :  VLAN_OPER_MODE_VLANTAG_OPER);

        if (!pConn->pAniVlanTagOpCfg && !pConn->pUniVlanTagOpCfg && pConn->p8021Map)
        {
            omci_GenNoTagWithDscpFilterRulePerIngress(pConn, index, flag, ingress);
        }

        pVlanRule->filterRule.filterCtagMode &= ~(VLAN_FILTER_NO_CARE_TAG);
        pVlanRule->filterRule.filterCtagMode |= VLAN_FILTER_PRI;
        pVlanRule->filterRule.filterCTag.pri = index;
        pVlanRule->filterRule.filterCTag.vid = OMCI_VID_FILTER_IGNORE;
    }

    if (pConn->pAniVlanTagOpCfg && !pConn->pUniVlanTagOpCfg)
        omci_MergeAniVlanTagOperRule(pConn, pVlanRule, pOutVlan);
    else if (!pConn->pAniVlanTagOpCfg && pConn->pUniVlanTagOpCfg)
    {
        pOutVlan->dsTagOperMode = (VTOCD_DS_VLAN_TAG_OP_MODE_REMOVE ==
            ((MIB_TABLE_VLANTAGOPCFGDATA_T *)(pConn->pUniVlanTagOpCfg))->DsTagOpMode ? VLAN_ACT_REMOVE :
            (((MIB_TABLE_VLANTAGOPCFGDATA_T *)(pConn->pUniVlanTagOpCfg))->DsTagOpMode ==
                ((MIB_TABLE_VLANTAGOPCFGDATA_T *)(pConn->pUniVlanTagOpCfg))->UsTagOpMode ?
                ((FAL_OK != feature_api_is_registered(FEATURE_API_BDP_00000008_GEN_RULE)) ?
                VLAN_ACT_TRANSPARENT : VLAN_ACT_REMOVE) : VLAN_ACT_REMOVE));
        pVlanRule->filterMode = VLAN_OPER_MODE_VLANTAG_OPER;
    }

    if (GOS_OK != omci_UpdateTrafficRuleByManual(pConn, index, pRule))
        goto fail_free;

    if (GOS_FAIL == feature_api(FEATURE_API_BDP_00000040, pRule))
        goto fail_free;

    if (NULL == pConn->pUniExtVlanCfg || TRUE == flag)
    {
        ret = (((pRule->dir == PON_GEMPORT_DIRECTION_DS) &&
                (NULL != pConn->pMcastGemIwTp) &&
                (GOS_OK != omci_McTransparentFwd(pConn, pRule))) ?
                omci_AddMcastTrafficRule(pConn, index, pRule) : omci_AddTrafficRule(pConn, index, pRule));

        if (GOS_OK == ret)
            return ret;
    }

fail_free:
    if(pRule)
        free(pRule);
    return ret;
}

static GOS_ERROR_CODE omci_GenNoVlanFilterRule(MIB_TREE_CONN_T *pConn, int index, BOOL flag)
{
    GOS_ERROR_CODE ret = GOS_OK;
    int ingress;

    ingress = omci_GetIngressByUniMbpcd(pConn);

    ret = omci_GenNoVlanFilterRulePerIngress(pConn, index, flag, ingress);

    return ret;
}


GOS_ERROR_CODE omci_GenVlanTagFilterRule(MIB_TREE_CONN_T *pConn,int index, BOOL flag)
{
    GOS_ERROR_CODE ret = GOS_OK;
    OMCI_VLANFILTER_MODE_T mode = OMCI_VLANFILTER_MODE_NOTSUPPORT;

    mode = omci_GetVlanFilterModeFromAniUni(pConn);

    switch(mode){
    case OMCI_VLANFILTER_MODE_FORWARDALL:
        ret = omci_GenNoVlanFilterRule(pConn,index, flag);
    break;
    case OMCI_VLANFILTER_MODE_DROPTAG_FOWARDUNTAG:
        ret = omci_GenForwardUnTagOnly(pConn,index, flag);
    break;
    case OMCI_VLANFILTER_MODE_FORWARDTAG_DROPUNTAG:
        ret = omci_GenForwardSingleTagOnly(pConn,index, flag);
    break;
    case OMCI_VLANFILTER_MODE_FORWARDLIST_VID_FORWARDUNTAG:
    case OMCI_VLANFILTER_MODE_FORWARDLIST_PRI_FORWARDUNTAG:
    case OMCI_VLANFILTER_MODE_FORWARDLIST_TCI_FORWARDUNTAG:
    {
        ret = omci_GenVlanFilterRule(pConn,mode,index, flag);
        ret = omci_GenForwardUnTagOnly(pConn,index, flag);
    }
    break;
    case OMCI_VLANFILTER_MODE_FORWARDLIST_VID_DROPUNTAG:
    case OMCI_VLANFILTER_MODE_FORWARDLIST_PRI_DROPUNTAG:
    case OMCI_VLANFILTER_MODE_FORWARDLIST_TCI_DROPUNTAG:
        ret = omci_GenVlanFilterRule(pConn,mode,index, flag);
    break;
    case OMCI_VLANFILTER_MODE_NOTSUPPORT:
        ret = GOS_FAIL;
    break;
    }

    return ret;
}


static GOS_ERROR_CODE omci_GenExtVlanRule(MIB_TREE_CONN_T *pConn, int index)
{
    MIB_TABLE_EXTVLANTAGOPERCFGDATA_T *pExtVlan;
    extvlanTableEntry_t *pEntry;
    int ret = GOS_OK;

    pExtVlan = pConn->pUniExtVlanCfg;
    LIST_FOREACH(pEntry,&pExtVlan->head,entries)
    {
        if (GOS_OK != omci_GenExtVlanSingleRule(pConn,index,pEntry))
        {
            ret = GOS_FAIL;
        }
    }
    return ret;
}


static GOS_ERROR_CODE omci_GenVlanTagOpRule(MIB_TREE_CONN_T *pConn, int index, BOOL flag)
{
    GOS_ERROR_CODE ret = GOS_OK;
    MIB_TABLE_VLANTAGOPCFGDATA_T *pVlanTagOpCfg;
    omci_vlan_rule_t *pRule = NULL;
    OMCI_VLAN_OPER_ts *pVlanRule;
    OMCI_VLAN_ACT_ts *pCVlanAct, *pSVlanAct;
    OMCI_VLAN_OUT_ts *pOutVlan;
    int ingress;

    pVlanTagOpCfg = (MIB_TABLE_VLANTAGOPCFGDATA_T *)pConn->pUniVlanTagOpCfg;
    ingress = omci_GetIngressByUniMbpcd(pConn);
    omci_CreateTraffRule(pConn, index, &pRule, ingress);

    if(!pRule)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "%s() %d,failed", __FUNCTION__, __LINE__);
        return  GOS_FAIL;
    }

    pVlanRule = &pRule->vlanRule;
    pCVlanAct = &pVlanRule->cTagAct;
    pSVlanAct = &pVlanRule->sTagAct;
    pOutVlan = &pVlanRule->outStyle;
    pVlanRule->filterMode = VLAN_OPER_MODE_VLANTAG_OPER;

    pOutVlan->dsTagOperMode = (VTOCD_DS_VLAN_TAG_OP_MODE_REMOVE == pVlanTagOpCfg->DsTagOpMode ? VLAN_ACT_REMOVE :
        (pVlanTagOpCfg->DsTagOpMode == pVlanTagOpCfg->UsTagOpMode ?
            ((FAL_OK != feature_api_is_registered(FEATURE_API_BDP_00000008_GEN_RULE)) ?
            VLAN_ACT_TRANSPARENT : VLAN_ACT_REMOVE) : VLAN_ACT_REMOVE));

    if (PON_GEMPORT_DIRECTION_DS == pRule->dir)
    {
        if (VTOCD_US_VLAN_TAG_OP_MODE_AS_IS != pVlanTagOpCfg->UsTagOpMode)
        {
            pVlanRule->filterRule.filterCtagMode = (VLAN_FILTER_VID | VLAN_FILTER_PRI);
            pVlanRule->filterRule.filterCTag.pri = (pVlanTagOpCfg->UsTagTci >> 13);
            pVlanRule->filterRule.filterCTag.vid = (0xfff & pVlanTagOpCfg->UsTagTci);
            memcpy(&pOutVlan->outVlan, &pVlanRule->filterRule.filterCTag, sizeof(OMCI_VLAN_ts));
        }

        if (VLAN_ACT_REMOVE == pOutVlan->dsTagOperMode)
        {
            pCVlanAct->vlanAct = VLAN_ACT_REMOVE;
        }

    }
    else
    {
        switch(pVlanTagOpCfg->UsTagOpMode)
        {
            case VTOCD_US_VLAN_TAG_OP_MODE_AS_IS:
                free(pRule);
                return omci_GenVlanTagFilterRule(pConn,index, flag);
            case VTOCD_US_VLAN_TAG_OP_MODE_MODIFY:
                pCVlanAct->vlanAct = VLAN_ACT_ADD;
                pCVlanAct->vidAct  = VID_ACT_ASSIGN;
                pCVlanAct->priAct  = PRI_ACT_ASSIGN;
                pCVlanAct->assignVlan.pri = (pVlanTagOpCfg->UsTagTci >> 13);
                pCVlanAct->assignVlan.vid = (0xfff & pVlanTagOpCfg->UsTagTci);
                pCVlanAct->assignVlan.tpid = UINT_MAX;
                pOutVlan->outTagNum = 1;
                memcpy(&pOutVlan->outVlan, &pCVlanAct->assignVlan, sizeof(OMCI_VLAN_ts));
                break;
            case VTOCD_US_VLAN_TAG_OP_MODE_INSERT:
                //change svlan tpid = 8100, do s-action
                pSVlanAct->vlanAct = VLAN_ACT_ADD;
                pSVlanAct->vidAct  = VID_ACT_ASSIGN;
                pSVlanAct->priAct  = PRI_ACT_ASSIGN;
                pSVlanAct->assignVlan.pri = (pVlanTagOpCfg->UsTagTci >> 13);
                pSVlanAct->assignVlan.vid = (0xfff & pVlanTagOpCfg->UsTagTci);
                pSVlanAct->assignVlan.tpid = 0x8100;
                pOutVlan->outTagNum = 2;
                memcpy(&pOutVlan->outVlan, &pSVlanAct->assignVlan, sizeof(OMCI_VLAN_ts));
                break;
            default:
                if (FAL_OK != feature_api_is_registered(FEATURE_API_BDP_00000008_GEN_RULE))
                {
                    free(pRule);
                    return omci_GenVlanTagFilterRule(pConn,index, flag);
                }
                else
                {
                    if (FAL_OK != feature_api(FEATURE_API_BDP_00000008_GEN_RULE, pVlanRule, pVlanTagOpCfg))
                    {
                        ret = GOS_FAIL;
                        goto fail_free;
                    }
                }
        }
    }

    if (pConn->pAniVlanTagFilter || pConn->pUniVlanTagFilter)
    {
        if(GOS_FAIL == (ret = omci_checkVlan(pConn, pRule)))
        {
            if (FAL_OK == feature_api(FEATURE_API_BDP_00000008_CHECK_MODE, pVlanTagOpCfg))
            {
                free(pRule);
                return omci_GenVlanTagFilterRule(pConn,index, flag);
            }
            goto fail_free;
        }
    }

    if (pConn->traffMode == OMCI_TRAF_MODE_8021P_BASE && pOutVlan->outVlan.pri != index)
    {
        ret = GOS_FAIL;
        goto fail_free;
    }

    /* change outer tag filter mode to include filter PBIT if 8021p mapper only map the specefic gem port  */
    omci_changeOuterTagFilterMode(pConn, index, pRule);

    if (NULL == pConn->pUniExtVlanCfg || TRUE == flag)
    {
        ret = (((pRule->dir == PON_GEMPORT_DIRECTION_DS) &&
                (NULL != pConn->pMcastGemIwTp) &&
                (GOS_OK != omci_McTransparentFwd(pConn, pRule))) ?
                omci_AddMcastTrafficRule(pConn, index, pRule) : omci_AddTrafficRule(pConn, index, pRule));
        if (GOS_OK == ret)
            return ret;
    }
fail_free:
    if(pRule)
        free(pRule);

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "%s() %d, ERR set vlan rule failed", __FUNCTION__, __LINE__);
    return ret;
}

GOS_ERROR_CODE omci_UpdateTrafficRuleByManual(MIB_TREE_CONN_T *pConn, int index, omci_vlan_rule_t *pRule)
{
    GOS_ERROR_CODE ret = GOS_OK;
    MIB_TABLE_PRIVATE_VLANCFG_T *pMibPrivateVlanCfg = NULL;

    omci_GetPrivateVlanCfgObj(&pMibPrivateVlanCfg);
    OMCI_ERR_CHK(OMCI_LOG_LEVEL_DBG, (!pMibPrivateVlanCfg), GOS_FAIL);

    if (PRIVATE_VLANCFG_TYPE_MANUAL_SETTING == pMibPrivateVlanCfg->Type)
    {
        switch(pMibPrivateVlanCfg->ManualMode)
        {
            case PRIVATE_VLANCFG_MANUAL_MODE_TRANSPAREN:
                break;
            case PRIVATE_VLANCFG_MANUAL_MODE_TAGGING:
                if (pMibPrivateVlanCfg->ManualTagVid == pRule->vlanRule.filterRule.filterCTag.vid)
                {
                    if (PON_GEMPORT_DIRECTION_DS == pRule->dir)
                    {
                        pRule->vlanRule.filterMode = VLAN_OPER_MODE_EXTVLAN;
                        pRule->vlanRule.filterRule.filterStagMode = VLAN_FILTER_NO_TAG;
                        pRule->vlanRule.filterRule.filterCtagMode = VLAN_FILTER_VID | VLAN_FILTER_PRI;
                        pRule->vlanRule.sTagAct.vlanAct = VLAN_ACT_REMOVE;
                        pRule->vlanRule.cTagAct.vlanAct = VLAN_ACT_REMOVE;
                        pRule->vlanRule.outStyle.outTagNum = 1;
                    }
                    else
                    {
                        pRule->vlanRule.filterMode = VLAN_OPER_MODE_EXTVLAN;
                        pRule->vlanRule.filterRule.filterStagMode = VLAN_FILTER_NO_TAG;
                        pRule->vlanRule.filterRule.filterCtagMode = VLAN_FILTER_NO_TAG;
                        pRule->vlanRule.sTagAct.vlanAct = VLAN_ACT_TRANSPARENT;
                        pRule->vlanRule.cTagAct.vlanAct = VLAN_ACT_ADD;
                        pRule->vlanRule.cTagAct.vidAct  = VID_ACT_ASSIGN;
                        pRule->vlanRule.cTagAct.priAct = PRI_ACT_ASSIGN;
                        pRule->vlanRule.cTagAct.assignVlan.vid = pMibPrivateVlanCfg->ManualTagVid;
                        pRule->vlanRule.cTagAct.assignVlan.pri = pMibPrivateVlanCfg->ManualTagPri;
                        pRule->vlanRule.outStyle.outTagNum = 1;
                        pRule->vlanRule.outStyle.outVlan.vid = pRule->vlanRule.cTagAct.assignVlan.vid;
                        pRule->vlanRule.outStyle.outVlan.pri = pRule->vlanRule.cTagAct.assignVlan.pri;
                    }
                }
                else
                {
                    ret = GOS_FAIL;
                }
                break;
            case PRIVATE_VLANCFG_MANUAL_MODE_REMOTE_ACCESS:
            case PRIVATE_VLANCFG_MANUAL_MODE_US_REMOVE_ACCESS_DS_UNTAG:
            default:
                // TBD
                break;

        }
    }
    OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "%s() %d, ret=%d, vid=(%u, %u)", __FUNCTION__, __LINE__, ret, pMibPrivateVlanCfg->ManualTagVid, pRule->vlanRule.filterRule.filterCTag.vid);
    return ret;
}



GOS_ERROR_CODE OMCI_GenTrafficRule(MIB_TREE_CONN_T *pConn,int index)
{
    GOS_ERROR_CODE ret = GOS_OK;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: Start ...!",__FUNCTION__);

    OMCI_ERR_CHK(OMCI_LOG_LEVEL_DBG, (!(pConn && pConn->pAniPort && pConn->pGemPortCtp)), GOS_FAIL);

    /*follow 984.4 implement*/
    /*1:M or All PASS*/
    if(!pConn->pAniExtVlanCfg && !pConn->pAniVlanTagFilter && !pConn->pUniExtVlanCfg && !pConn->pUniVlanTagFilter && !pConn->pUniVlanTagOpCfg)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, " *********[N:1]*********** ");
        ret = omci_GenNoVlanFilterRule(pConn,index, FALSE);
        if (ret != GOS_OK)
        {
            OMCI_LOG(
                OMCI_LOG_LEVEL_WARN,
                "Line %u, Failed to GenNoVlanFilterRule, ret0x%X",
                __LINE__,
                ret);
            return ret;
        }
    }else
    /*1:P or 1:MP, and VLAN operation*/
    if((pConn->pAniVlanTagFilter || pConn->pUniVlanTagFilter)&& pConn->pUniExtVlanCfg)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"*********[VLAN+EXT]***********");
        ret = omci_GenExtVlanRule(pConn,index);
        // if ret is GOS_ERR_STATE means ext vlan rule treat is not equal to ani rule or is default drop rule
        if (GOS_OK == ret && LIST_EMPTY(&pConn->pUniExtVlanCfg->head))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_WARN,"*********[VLAN+EXT]  no ext vlan entry ...***********");
            ret = omci_GenVlanTagFilterRule(pConn,index, TRUE);
            if (ret != GOS_OK)
            {
                OMCI_LOG(
                    OMCI_LOG_LEVEL_WARN,
                    "Line %u, Failed to GenVlanTagFilterRule, ret0x%X",
                    __LINE__,
                    ret);
                return ret;
            }
        }
    }else
    /*1:P or 1:MP, and no VLAN operation*/
    if(pConn->pUniExtVlanCfg)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"*********[EXT]*********** ");
        ret = omci_GenExtVlanRule(pConn,index);
        // if ret is GOS_ERR_STATE means ext vlan rule treat is not equal to ani rule or is default drop rule
        if (GOS_OK == ret && LIST_EMPTY(&pConn->pUniExtVlanCfg->head))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_WARN,"*********[EXT]  no ext vlan entry ...***********");
            ret = omci_GenNoVlanFilterRule(pConn,index, TRUE);
            if (ret != GOS_OK)
            {
                OMCI_LOG(
                    OMCI_LOG_LEVEL_WARN,
                    "Line %u, Failed to GenNoVlanTagFilterRule, ret0x%X",
                    __LINE__,
                    ret);
                return ret;
            }
        }
    }else
    /* 1:P or 1:MP with a UNI VLAN operation */
    if(!pConn->pAniVlanTagOpCfg && pConn->pUniVlanTagOpCfg)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            " *********[VLAN] + VLAN Op Cfg *********** ");
        ret = omci_GenVlanTagOpRule(pConn, index, FALSE);
        if (ret != GOS_OK)
        {
            OMCI_LOG(
                OMCI_LOG_LEVEL_WARN,
                "Line %u, Failed to GenVlanTagOpRule, ret0x%X",
                __LINE__,
                ret);
            return ret;
        }
    }
    else if(pConn->pAniVlanTagFilter || pConn->pUniVlanTagFilter)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG," *********[VLAN]*********** ");
        ret = omci_GenVlanTagFilterRule(pConn,index, FALSE);
        if (ret != GOS_OK)
        {
            OMCI_LOG(
                OMCI_LOG_LEVEL_WARN,
                "Line %u, Failed to GenVlanTagFilterRule, ret0x%X",
                __LINE__,
                ret);
            return ret;
        }
    }

    /* apply conns. there is no conns if above return value is error */
    ret = omci_ApplyTrafficRule(pConn,index);

    return ret;
}

GOS_ERROR_CODE omci_is_notify_suppressed_by_circuitpack(omci_me_instance_t  slotID,
                                                        BOOL                *pSuppressed)
{
    GOS_ERROR_CODE              ret;
    MIB_TABLE_INDEX             tableIndex;
    omci_me_instance_t          instanceID;
    MIB_TABLE_ONTG_T            mibOntG;
    MIB_TABLE_CIRCUITPACK_T     mibCircuitPack;

    if (!pSuppressed)
        return GOS_ERR_PARAM;

    *pSuppressed = FALSE;

    // check ONU-G
    tableIndex = MIB_TABLE_ONTG_INDEX;
    instanceID = TXC_ONUG_INSTANCE_ID;
    mibOntG.EntityID = instanceID;
    ret = MIB_Get(tableIndex, &mibOntG, MIB_GetTableEntrySize(tableIndex));
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        return ret;
    }

    // once we found it's locked, return immediately
    if (OMCI_ME_ATTR_ADMIN_STATE_LOCK == mibOntG.AdminState)
    {
        *pSuppressed = TRUE;
        goto out;
    }

    // check Circuit pack
    // slotID 0xFF is reserved by G.988
    if (0xFF != (slotID & 0xFF))
    {
        tableIndex = MIB_TABLE_CIRCUITPACK_INDEX;
        instanceID = slotID;
        mibCircuitPack.EntityID = instanceID;
        ret = MIB_Get(tableIndex, &mibCircuitPack, MIB_GetTableEntrySize(tableIndex));
        if (GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Instance not found in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
            return ret;
        }

        // once we found it's locked, return immediately
        if (OMCI_ME_ATTR_ADMIN_STATE_LOCK == mibCircuitPack.AdminState)
        {
            *pSuppressed = TRUE;
            goto out;
        }
    }

out:
    return GOS_OK;
}

GOS_ERROR_CODE omci_AvcCallback(MIB_TABLE_INDEX     tableIndex,
                                void                *pOldRow,
                                void                *pNewRow,
                                MIB_ATTRS_SET       *pAttrsSet,
                                MIB_OPERA_TYPE      operationType)
{
    MIB_ATTR_INDEX              attrIndex;
    UINT32                      i, attrSize;
    MIB_ATTRS_SET               avcAttrSet;
    omci_me_instance_t          entityID;
    omci_msg_norm_baseline_t    omciMsg;
    omci_msg_attr_mask_t        avcOmciSet;
    GOS_ERROR_CODE              ret;
    CHAR*                       pData;

    if (!pAttrsSet)
        return GOS_ERR_PARAM;

    if (MIB_SET == operationType)
    {
        if (!pOldRow || !pNewRow)
            return GOS_ERR_PARAM;
    }
    else if (MIB_DEL == operationType)
    {
        if (!pOldRow)
            return GOS_ERR_PARAM;
    }
    else if (MIB_ADD == operationType)
    {
        if (!pNewRow)
            return GOS_ERR_PARAM;
    }
    else
        return GOS_OK;

    MIB_ClearAttrSet(&avcAttrSet);

    MIB_GetAttrFromBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &entityID, pNewRow ? pNewRow : pOldRow, sizeof(omci_me_instance_t));
    omciMsg.meId.meInstance = entityID;
    omciMsg.meId.meClass = MIB_GetTableClassId(tableIndex);
    omciMsg.type = OMCI_MSG_TYPE_ATTR_VALUE_CHANGE;
    memset(&omciMsg.content[0], 0x00, OMCI_MSG_BASELINE_CONTENTS_LEN);

    pData = (CHAR*)(&omciMsg.content[2]);

    for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0; i < MIB_GetTableAttrNum(tableIndex);
         i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        // Not in attribute set or Not a AVC attribute
        if (!MIB_IsInAttrSet(pAttrsSet, attrIndex) || (!MIB_GetAttrAvcFlag(tableIndex, attrIndex)))
        {
            continue;
        }

        attrSize = MIB_GetAttrSize(tableIndex, attrIndex);
        ret      = GOS_FAIL;

        switch (MIB_GetAttrDataType(tableIndex, attrIndex))
        {
            case MIB_ATTR_TYPE_UINT8:
            {
                UINT8 value = 0, oldValue = 0;
                if (pNewRow)
                MIB_GetAttrFromBuf(tableIndex, attrIndex, &value, pNewRow, attrSize);
                if (pOldRow)
                MIB_GetAttrFromBuf(tableIndex, attrIndex, &oldValue, pOldRow, attrSize);
                // Make sure that value is changed
                if (!pNewRow || !pOldRow || value != oldValue)
                {
                    ret = GOS_OK;
                    memcpy(pData, &value, attrSize);
                }
                break;
            }
            case MIB_ATTR_TYPE_STR:
            {
                int isDiffStr = 0;
                CHAR * pOldVal = (CHAR *)malloc(sizeof(CHAR)*attrSize);
                CHAR * pNewVal = (CHAR *)malloc(sizeof(CHAR)*attrSize);

                if(pOldVal)
                    memset(pOldVal, 0x0, sizeof(CHAR)*attrSize);
                if(pNewVal)
                    memset(pNewVal, 0x0, sizeof(CHAR)*attrSize);

                if (pNewRow && pNewVal)
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, pNewVal, pNewRow, attrSize);
                if (pOldRow && pOldVal)
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, pOldVal, pOldRow, attrSize);
                if(pNewVal && pOldVal)
                    isDiffStr = strcmp(pOldVal, pNewVal);

                // Make sure that value is changed
                if (!pNewRow || !pOldRow || 0 != isDiffStr)
                {
                    ret = GOS_OK;
                    if(pNewVal)
                        memcpy(pData, pNewVal, attrSize);
                }
                if(pOldVal)
                    free(pOldVal);
                if(pNewVal)
                    free(pNewVal);
                break;
            }
            case MIB_ATTR_TYPE_UINT16:
            {
                UINT16 value = 0, oldValue = 0;
                if (pNewRow)
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &value, pNewRow, attrSize);
                if (pOldRow)
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &oldValue, pOldRow, attrSize);
                // Make sure that value is changed
                if (!pNewRow || !pOldRow || value != oldValue)
                {
                    ret = GOS_OK;
                    GOS_SetUINT16((UINT16*)pData, GOS_Htons(value));
                }
                break;
            }
            case MIB_ATTR_TYPE_UINT32:
            {
                UINT32 value = 0, oldValue = 0;
                if (pNewRow)
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &value, pNewRow, attrSize);
                if (pOldRow)
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &oldValue, pOldRow, attrSize);
                // Make sure that value is changed
                if (!pNewRow || !pOldRow || value != oldValue)
                {
                    ret = GOS_OK;
                    GOS_SetUINT32((UINT32*)pData, GOS_Htonl(value));
                }
                break;
            }
            case MIB_ATTR_TYPE_UINT64:
            {
                UINT64 value, oldValue;
                if (pNewRow)
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &value, pNewRow, attrSize);
                if (pOldRow)
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &oldValue, pOldRow, attrSize);
                // Make sure that value is changed
                if (!pNewRow || !pOldRow || (value.high != oldValue.high) || (value.low != oldValue.low))
                {
                    ret = GOS_OK;
                    value = GOS_Htonll(value);
                    GOS_UINT64ToBuff(value, pData, MIB_GetAttrLen(tableIndex, attrIndex));
                }
                break;
            }
            case MIB_ATTR_TYPE_TABLE:
                // table attr only raise AVC
                // without send out the contents
                attrSize = 0;
                ret = GOS_OK;
                break;

            default:
                break;
        }

        if (GOS_OK == ret)
        {
            // make sure that zero padding should be used in meaningless bytes
            memset(pData + attrSize, 0, OMCI_MSG_BASELINE_CONTENTS_LEN - 2 - attrSize);

            MIB_ClearAttrSet(&avcAttrSet);
            MIB_SetAttrSet(&avcAttrSet, attrIndex);
            OMCI_AttrsMibSetToOmciSet(&avcAttrSet, &avcOmciSet);
            GOS_SetUINT16((UINT16*)&omciMsg.content[0], GOS_Htons((UINT16)avcOmciSet));
            OMCI_SendIndicationToOlt(&omciMsg, 0);
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: Class %d, Entity 0x%x, attr %#x value changed indication",__FUNCTION__,
                        omciMsg.meId.meClass, omciMsg.meId.meInstance, avcOmciSet);
        }
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_arc_timer_task(void  *pData)
{
    UINT16              classID;
    UINT16              instanceID;
    MIB_ATTR_INDEX      arcIndex;
    MIB_TABLE_INDEX     tableIndex;
    MIB_ENTRY_T         *pMibEntry;
    UINT32              dataSize;
    void                *pOldRow;
    UINT8               arcState;
    MIB_ATTRS_SET       attrSet;

    // make sure the data is available
    if (!pData)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "arc data unavailable to proceed");

        return GOS_ERR_PARAM;
    }

    // fetch encapsulated arguments
    memcpy(&classID, pData, sizeof(UINT16));
    pData += sizeof(UINT16);
    memcpy(&instanceID, pData, sizeof(UINT16));
    pData += sizeof(UINT16);
    memcpy(&arcIndex, pData, sizeof(MIB_ATTR_INDEX));
    pData = pData - sizeof(UINT16) - sizeof(UINT16);

    tableIndex = MIB_GetTableIndexByClassId(classID);
    dataSize = MIB_GetTableEntrySize(tableIndex);

    // allocate memory for data backup
    pOldRow = malloc(dataSize);
    if (!pOldRow)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Allocate memory for arc handler fail: %s",
            MIB_GetTableName(tableIndex));

        goto free_arg;
    }

    // search MIB for original entry
    pMibEntry = mib_FindEntryByInstanceID(tableIndex, instanceID);
    if (!pMibEntry)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found: %s, 0x%x",
            MIB_GetTableName(tableIndex), instanceID);

        goto free_mib;
    }

    // backup entry data
    memcpy(pOldRow, pMibEntry->pData, dataSize);

    // turn ARC state to disable
    arcState = OMCI_ME_ATTR_ARC_DISABLED;
    MIB_SetAttrToBuf(tableIndex, arcIndex, &arcState, pMibEntry->pData, sizeof(arcState));

    // invoke AVC callback
    MIB_ClearAttrSet(&attrSet);
    MIB_SetAttrSet(&attrSet, arcIndex);
    omci_AvcCallback(tableIndex, pOldRow, pMibEntry->pData, &attrSet, MIB_SET);

    // invoke ARC processor
    OMCI_MeOperCfg(tableIndex, pOldRow, pMibEntry->pData, MIB_SET, attrSet, OMCI_MSG_PRI_NORMAL);

free_mib:
    // free allocated memory
    free(pOldRow);

free_arg:
    // free pre-allocated memory before leaving
    free(pData);

    return GOS_OK;
}

static void omci_arc_timer_default_handler(UINT16           classID,
                                            UINT16          instanceID,
                                            MIB_ATTR_INDEX  arcIndex)
{
    CHAR    taskName[OMCI_TASK_NAME_MAX_LEN];
    void    *pData = NULL;

    // copy argument in order to pass it further
    pData = malloc(sizeof(UINT16) + sizeof(UINT16) + sizeof(MIB_ATTR_INDEX));
    if (!pData)
        return;
    memcpy(pData, &classID, sizeof(UINT16));
    pData += sizeof(UINT16);
    memcpy(pData, &instanceID, sizeof(UINT16));
    pData += sizeof(UINT16);
    memcpy(pData, &arcIndex, sizeof(MIB_ATTR_INDEX));
    pData = pData - sizeof(UINT16) - sizeof(UINT16);

    snprintf(taskName, OMCI_TASK_NAME_MAX_LEN, "arc on %d/%#x", classID, instanceID);

    // spawn task for arc handler
    OMCI_SpawnTask(taskName,
                    omci_arc_timer_task,
                    pData,
                    OMCI_TASK_PRI_TIMER,
                    FALSE);
}

GOS_ERROR_CODE omci_arc_timer_processor(MIB_TABLE_INDEX     tableIndex,
                                        void                *pOldRow,
                                        void                *pNewRow,
                                        MIB_ATTR_INDEX      arcIndex,
                                        MIB_ATTR_INDEX      arcIntervalIndex)
{
    GOS_ERROR_CODE      ret = GOS_OK;
    omci_timer_entry_t  *pEntry;
    omci_me_class_t     classID;
    omci_me_instance_t  instanceID;
    UINT8               newArcState, oldArcState;

    if (!pOldRow || !pNewRow)
        return GOS_ERR_PARAM;

    classID = MIB_GetTableClassId(tableIndex);

    // read out the instanceID
    MIB_GetAttrFromBuf(tableIndex,
        MIB_ATTR_FIRST_INDEX, &instanceID, pNewRow, sizeof(omci_me_instance_t));
    // read out the ARC state
    MIB_GetAttrFromBuf(tableIndex, arcIndex, &newArcState, pNewRow, sizeof(UINT8));
    MIB_GetAttrFromBuf(tableIndex, arcIndex, &oldArcState, pOldRow, sizeof(UINT8));

    pEntry = omci_timer_search(classID, instanceID);

    if (newArcState != oldArcState)
    {
        // disable ARC = deactivate & delete timer
        if (OMCI_ME_ATTR_ARC_DISABLED == newArcState)
            ret = omci_timer_delete_by_entry(pEntry);
        else
        {
            UINT8   arcInterval;

            // read out the ARC interval
            MIB_GetAttrFromBuf(tableIndex,
                arcIntervalIndex, &arcInterval, pNewRow, sizeof(UINT8));

            // enable ARC = create & activate timer
            // nanoSecs set to 1 to prevent all zero condition
            ret = omci_timer_create(classID, instanceID,
                arcInterval * OMCI_ALARM_REPORT_CTRL_INTERVAL_SCALE_FACTOR,
                1, FALSE, arcIndex, omci_arc_timer_default_handler,OMCI_TIMER_SIG_ARC);
        }
    }

    return ret;
}

GOS_ERROR_CODE pptp_eth_uni_switch_port_to_me_id(UINT16             portId,
                                                omci_me_instance_t  *pInstanceID)
{
    UINT8   slotId = 0;
    UINT8   portIdInType;

    if (!pInstanceID || portId >= RTK_MAX_NUM_OF_PORTS)
        return GOS_ERR_PARAM;

    if (RT_FE_PORT == gInfo.devCapabilities.ethPort[portId].portType)
    {
        slotId = gInfo.txc_cardhld_fe_slot_type_id;
        portIdInType = gInfo.devCapabilities.ethPort[portId].portIdInType + 1;
    }

    if (RT_GE_PORT == gInfo.devCapabilities.ethPort[portId].portType)
    {
        slotId = TXC_CARDHLD_GE_SLOT_TYPE_ID;
        portIdInType = gInfo.devCapabilities.ethPort[portId].portIdInType + 1;
    }

    if (0 == slotId)
        return GOS_FAIL;

    *pInstanceID = (slotId << 8) | portIdInType;

    return GOS_OK;
}

GOS_ERROR_CODE 
pptp_eth_uni_me_id_to_switch_port(
    omci_me_instance_t instanceID,
                                                UINT16              *pPortId)
{
    UINT8   slotId;
    UINT8   portId, recvPortId;

    if (!pPortId)
        return GOS_ERR_PARAM;

    slotId = instanceID >> 8;

    if (gInfo.txc_cardhld_fe_slot_type_id != slotId &&
            TXC_CARDHLD_GE_SLOT_TYPE_ID != slotId)
        return GOS_FAIL;

    recvPortId = (instanceID & 0xFF) - 1;

    for (portId = 0; portId < RTK_MAX_NUM_OF_PORTS; portId++)
    {
        if (gInfo.devCapabilities.ethPort[portId].portIdInType == recvPortId)
        {
            if (FAL_ERR_NOT_REGISTER == feature_api_is_registered(FEATURE_API_ME_00020000))
            {
                if (gInfo.txc_cardhld_fe_slot_type_id == slotId &&
                        RT_FE_PORT != gInfo.devCapabilities.ethPort[portId].portType)
                    continue;
                if (TXC_CARDHLD_GE_SLOT_TYPE_ID == slotId &&
                        RT_GE_PORT != gInfo.devCapabilities.ethPort[portId].portType)
                    continue;
            }
            *pPortId = portId;

            break;
        }
    }

    return GOS_OK;
}

GOS_ERROR_CODE omci_get_all_fe_eth_uni_port_mask(UINT32 *pPortMask)
{
    UINT8   portId;

    if (!pPortMask)
        return GOS_ERR_PARAM;

    *pPortMask = 0;

    for (portId = 0; portId < RTK_MAX_NUM_OF_PORTS; portId++)
    {
        if (gInfo.devCapabilities.ethPort[portId].portType == RT_FE_PORT)
        {
            *pPortMask |= (1 << portId);
        }
    }

    return GOS_OK;
}

GOS_ERROR_CODE omci_get_all_ge_eth_uni_port_mask(UINT32 *pPortMask)
{
    UINT8   portId;

    if (!pPortMask)
        return GOS_ERR_PARAM;

    *pPortMask = 0;

    for (portId = 0; portId < RTK_MAX_NUM_OF_PORTS; portId++)
    {
        if (gInfo.devCapabilities.ethPort[portId].portType == RT_GE_PORT)
        {
            *pPortMask |= (1 << portId);
        }
    }

    return GOS_OK;
}

GOS_ERROR_CODE omci_get_all_eth_uni_port_mask(UINT32 *pPortMask)
{
    UINT32  fePortMask;
    UINT32  gePortMask;

    if (!pPortMask)
        return GOS_ERR_PARAM;

    omci_get_all_fe_eth_uni_port_mask(&fePortMask);
    omci_get_all_ge_eth_uni_port_mask(&gePortMask);

    *pPortMask = fePortMask | gePortMask;

    return GOS_OK;
}

GOS_ERROR_CODE omci_get_eth_uni_port_mask_behind_veip(UINT16 mbspId, UINT32 *pPortMask)
{
    GOS_ERROR_CODE          ret;
    MIB_TREE_T              *pTree;
    MIB_NODE_T              *pNode;
    MIB_TREE_NODE_ENTRY_T   *pNodeEntry;
    MIB_ENTRY_T             *pEntry;
    MIB_TABLE_VEIP_T        *pMibVeip;
    MIB_TABLE_UNIG_T        mibUniG;
    UINT16                  portId;

    if (!pPortMask)
        return GOS_ERR_PARAM;

    *pPortMask = 0;

    pTree = MIB_AvlTreeSearchByKey(NULL, mbspId, AVL_KEY_MACBRISERVPROF);
    if (pTree)
    {
        pNode = MIB_AvlTreeSearch(pTree->root, AVL_KEY_VEIP);
        if (pNode)
        {
            LIST_FOREACH(pNodeEntry, &pNode->data.treeNodeEntryHead, treeNodeEntry)
            {
                pEntry = pNodeEntry->mibEntry;
                pMibVeip = (MIB_TABLE_VEIP_T *)pEntry->pData;

                ret = MIB_GetFirst(MIB_TABLE_UNIG_INDEX, &mibUniG, sizeof(MIB_TABLE_UNIG_T));
                while (GOS_OK == ret)
                {
                    if (mibUniG.NonOmciPointer == pMibVeip->EntityId)
                    {
                        if (GOS_OK == pptp_eth_uni_me_id_to_switch_port(mibUniG.EntityID, &portId))
                        {
                            *pPortMask |= (1 << portId);
                        }
                    }

                    ret = MIB_GetNext(MIB_TABLE_UNIG_INDEX, &mibUniG, sizeof(MIB_TABLE_UNIG_T));
                }
            }
        }
    }

    return GOS_OK;
}

void omci_get_channel_index_by_pots_uni_me_id(omci_me_instance_t    instanceID,
                                              UINT16                *pChannelID)
{
    UINT8   slotId;
    UINT16  start_id;

    slotId      = TXC_GET_SLOT_NUM_BY_SLOT_ID(TXC_CARDHLD_POTS_SLOT);
    start_id    = (UINT16)((slotId << 8) | 0x1);
    *pChannelID = instanceID - start_id;

    return;
}

static omci_RTPPM_history_data_t      *gpRtpCntrs     = NULL;
static omci_CCPM_history_data_t*    gpCcpmCntrs = NULL;
static omci_SIPAGPM_history_data_t* gpSipagpmCntrs = NULL;
static omci_SIPCIPM_history_data_t* gpSipcipmCntrs = NULL;
static omci_port_stat_t     *gpEthPortCntrs = NULL;
static omci_flow_stat_t     *gpGemPortCntrs = NULL;
static UINT32               gGemPortEntryCnt = 0;

GOS_ERROR_CODE omci_pm_update_pptp_eth_uni(omci_pm_oper_type_t  operType)
{
    GOS_ERROR_CODE          ret;
    MIB_TABLE_INDEX         tableIndex;
    UINT32                  entrySize;
    UINT32                  entryCnt;
    MIB_TABLE_ETHUNI_T      mibPptpEthUNI;
    UINT16                  portId;
    omci_port_stat_t        *pEthPortCntrs;

    tableIndex = MIB_TABLE_ETHUNI_INDEX;
    entrySize = MIB_GetTableEntrySize(tableIndex);
    entryCnt = MIB_GetTableCurEntryCount(tableIndex);

    // allocate memory if it's not created yet
    if (!gpEthPortCntrs)
    {
        gpEthPortCntrs = malloc(entryCnt * sizeof(omci_port_stat_t));
        if (!gpEthPortCntrs)
            return GOS_FAIL;
    }
    pEthPortCntrs = gpEthPortCntrs;

    // start collect counters for all ports
    ret = MIB_GetFirst(tableIndex, &mibPptpEthUNI, entrySize);
    while (GOS_OK == ret)
    {
        if (GOS_OK == pptp_eth_uni_me_id_to_switch_port(mibPptpEthUNI.EntityID, &portId))
        {
            // always update PM values
            /*critical section*/
            OMCI_GET_PM_ETH_LOCK;
            pEthPortCntrs->port = portId;

            omci_wrapper_getPortStat(pEthPortCntrs);
            OMCI_GET_PM_ETH_UNLOCK;


            if (OMCI_PM_OPER_SWAP == operType ||
                    OMCI_PM_OPER_UPDATE_AND_SWAP == operType ||
                    OMCI_PM_OPER_RESET == operType)
            {
                // reset PM values
                omci_wrapper_resetPortStat(portId);
            }
        }

        pEthPortCntrs++;
        ret = MIB_GetNext(tableIndex, &mibPptpEthUNI, entrySize);
    }

    return GOS_OK;
}

GOS_ERROR_CODE omci_pm_getcurrent_pptp_eth_uni(omci_me_instance_t   instanceID,
                                                omci_port_stat_t                *pCntrs)
{
    GOS_ERROR_CODE          ret;
    MIB_TABLE_INDEX         tableIndex;
    UINT32                  entrySize;
    MIB_TABLE_ETHUNI_T      mibPptpEthUNI;
    omci_port_stat_t        *pEthPortCntrs;

    if (!pCntrs)
        return GOS_FAIL;

    tableIndex = MIB_TABLE_ETHUNI_INDEX;
    entrySize = MIB_GetTableEntrySize(tableIndex);

    pEthPortCntrs = gpEthPortCntrs;
    if (!pEthPortCntrs)
        return GOS_FAIL;

    // start search counter for specified port
    ret = MIB_GetFirst(tableIndex, &mibPptpEthUNI, entrySize);
    while (GOS_OK == ret)
    {
        if (mibPptpEthUNI.EntityID != instanceID)
            goto get_next;

        OMCI_GET_PM_ETH_LOCK;
        *pCntrs = *pEthPortCntrs;
        OMCI_GET_PM_ETH_UNLOCK;

        return GOS_OK;

get_next:
        pEthPortCntrs++;
        ret = MIB_GetNext(tableIndex, &mibPptpEthUNI, entrySize);
    }

    return GOS_ERR_NOT_FOUND;
}

GOS_ERROR_CODE omci_pm_update_gem_port(omci_pm_oper_type_t  operType)
{
    GOS_ERROR_CODE              ret;
    MIB_TABLE_INDEX             tableIndex;
    UINT32                      entrySize;
    UINT32                      entryCnt;
    MIB_TABLE_GEMPORTCTP_T      mibGemPort;
    UINT16                      dsFlowID;
    UINT16                      usFlowID;
    omci_flow_stat_t            *pGemPortCntrs;

    tableIndex = MIB_TABLE_GEMPORTCTP_INDEX;
    entrySize = MIB_GetTableEntrySize(tableIndex);
    entryCnt = MIB_GetTableCurEntryCount(tableIndex);

    // allocate memory if it's not created yet
    if (!gpGemPortCntrs || gGemPortEntryCnt < entryCnt)
    {
        pGemPortCntrs = realloc(gpGemPortCntrs,
                                // counter is separated for US/DS, multiply 2
                                entryCnt * sizeof(omci_flow_stat_t) * 2);
        if (!pGemPortCntrs)
            return GOS_FAIL;

        gGemPortEntryCnt = entryCnt;
        gpGemPortCntrs = pGemPortCntrs;
    }
    else
        pGemPortCntrs = gpGemPortCntrs;

    // start collect counters for all ports
    ret = MIB_GetFirst(tableIndex, &mibGemPort, entrySize);
    while (GOS_OK == ret)
    {
        // get ds flow id
        if (GPNC_DIRECTION_ANI_TO_UNI == mibGemPort.Direction ||
                GPNC_DIRECTION_BIDIRECTION == mibGemPort.Direction)
        {
            dsFlowID = omci_GetDsFlowIdByPortId(mibGemPort.PortID);

            if (gInfo.devCapabilities.totalGEMPortNum != dsFlowID)
            {
                pGemPortCntrs->flow = dsFlowID;

                OMCI_GET_PM_GEM_LOCK;
                pGemPortCntrs->flow = dsFlowID;
                // update/reset PM values
                omci_wrapper_getDsFlowStat(pGemPortCntrs);
                OMCI_GET_PM_GEM_UNLOCK;
            }
        }

        pGemPortCntrs++;

        // get us flow id
        if (GPNC_DIRECTION_UNI_TO_ANI == mibGemPort.Direction ||
                GPNC_DIRECTION_BIDIRECTION == mibGemPort.Direction)
        {
            usFlowID = omci_GetUsFlowIdByPortId(mibGemPort.PortID);

            if (gInfo.devCapabilities.totalGEMPortNum != usFlowID)
            {
                pGemPortCntrs->flow = usFlowID;

                OMCI_GET_PM_GEM_LOCK;
                pGemPortCntrs->flow = usFlowID;
                // update/reset PM values
                omci_wrapper_getUsFlowStat(pGemPortCntrs);
                OMCI_GET_PM_GEM_UNLOCK;

            }
        }

        pGemPortCntrs++;
        ret = MIB_GetNext(tableIndex, &mibGemPort, entrySize);
    }

    return GOS_OK;
}

GOS_ERROR_CODE omci_pm_getcurrent_gem_port(omci_me_instance_t   instanceID,
                                            omci_flow_stat_t                *pCntrs)
{
    GOS_ERROR_CODE              ret;
    MIB_TABLE_INDEX             tableIndex;
    UINT32                      entrySize;
    MIB_TABLE_GEMPORTCTP_T      mibGemPort;
    omci_flow_stat_t            *pGemPortCntrs;

    if (!pCntrs)
        return GOS_FAIL;

    tableIndex = MIB_TABLE_GEMPORTCTP_INDEX;
    entrySize = MIB_GetTableEntrySize(tableIndex);

    pGemPortCntrs = gpGemPortCntrs;
    if (!pGemPortCntrs)
        return GOS_FAIL;

    // start search counter for specified port
    ret = MIB_GetFirst(tableIndex, &mibGemPort, entrySize);
    while (GOS_OK == ret)
    {
        if (mibGemPort.EntityID != instanceID)
            goto get_next;

        OMCI_GET_PM_GEM_LOCK;
        memcpy(pCntrs,pGemPortCntrs,sizeof(omci_flow_stat_t)*2);
        OMCI_GET_PM_GEM_UNLOCK;

        return GOS_OK;

get_next:
        pGemPortCntrs += 2;
        ret = MIB_GetNext(tableIndex, &mibGemPort, entrySize);
    }

    return GOS_ERR_NOT_FOUND;
}

GOS_ERROR_CODE omci_pm_update_rtp(omci_pm_oper_type_t  operType)
{
    GOS_ERROR_CODE          ret;
    MIB_TABLE_INDEX         tableIndex;
    UINT32                  entrySize;
    UINT32                  entryCnt;
    MIB_TABLE_POTSUNI_T     mibPots;
    UINT16                  chid;
    omci_RTPPM_history_data_t        *pRtpCntrs;
    MIB_TABLE_INFO_T       *pTableInfo = NULL;

    pTableInfo = MIB_GetTableInfoPtr(MIB_TABLE_POTSUNI_INDEX);
    if (!pTableInfo)
        return GOS_OK;

    if (0 == MIB_GetTableCurEntryCount(MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_INDEX))
        return GOS_OK;

    tableIndex = MIB_TABLE_POTSUNI_INDEX;
    entrySize = MIB_GetTableEntrySize(tableIndex);
    entryCnt = MIB_GetTableCurEntryCount(tableIndex);

    // allocate memory if it's not created yet
    if (!gpRtpCntrs)
    {
        gpRtpCntrs = malloc(entryCnt * sizeof(omci_RTPPM_history_data_t));
        if (!gpRtpCntrs)
            return GOS_FAIL;
    }
    pRtpCntrs = gpRtpCntrs;

    // start collect counters for all ports
    ret = MIB_GetFirst(tableIndex, &mibPots, entrySize);
    while (GOS_OK == ret)
    {
        omci_get_channel_index_by_pots_uni_me_id(mibPots.EntityId, &chid);

        if ( chid < gInfo.devCapabilities.potsPortNum )
        {
            memset(pRtpCntrs, 0, sizeof(omci_RTPPM_history_data_t));

            // always update PM values
            OMCI_GET_PM_RTP_LOCK;
            pRtpCntrs->ch_id = chid;

            VOICE_WRAPPER(omci_voice_RTPPM_history_data_get,  pRtpCntrs);

            OMCI_GET_PM_RTP_UNLOCK;

            // TBD: The below code should be removed if rtp stat is read_clear.
            if (OMCI_PM_OPER_SWAP == operType ||
                    OMCI_PM_OPER_UPDATE_AND_SWAP == operType ||
                    OMCI_PM_OPER_RESET == operType)
            {
                // reset PM values
                VOICE_WRAPPER(omci_voice_rtp_stat_clear, chid);
            }
        }

        pRtpCntrs++;
        ret = MIB_GetNext(tableIndex, &mibPots, entrySize);
    }

    return GOS_OK;
}

GOS_ERROR_CODE omci_pm_getcurrent_rtp(omci_me_instance_t    instanceID,
                                      omci_RTPPM_history_data_t                  *pCntrs)
{
    GOS_ERROR_CODE                                      ret;
    MIB_TABLE_INDEX                                     tableIndex;
    UINT32                                              entrySize;
    MIB_TABLE_POTSUNI_T                                 mibPots;
    omci_RTPPM_history_data_t                                       *pRtpCntrs;
    MIB_TABLE_INFO_T                                    *pTableInfo = NULL;

    pTableInfo = MIB_GetTableInfoPtr(MIB_TABLE_POTSUNI_INDEX);
    if (!pTableInfo)
        return GOS_OK;


    if (!pCntrs)
        return GOS_FAIL;

    tableIndex = MIB_TABLE_POTSUNI_INDEX;
    entrySize = MIB_GetTableEntrySize(tableIndex);

    pRtpCntrs = gpRtpCntrs;
    if (!pRtpCntrs)
        return GOS_FAIL;

    // start search counter for specified port
    ret = MIB_GetFirst(tableIndex, &mibPots, entrySize);
    while (GOS_OK == ret)
    {
        if (mibPots.EntityId != instanceID)
            goto get_next;

        OMCI_GET_PM_RTP_LOCK;
        *pCntrs = *pRtpCntrs;
        OMCI_GET_PM_RTP_UNLOCK;

        return GOS_OK;

get_next:
        pRtpCntrs++;
        ret = MIB_GetNext(tableIndex, &mibPots, entrySize);
    }

    return GOS_ERR_NOT_FOUND;
}

GOS_ERROR_CODE
omci_pm_update_ccpm (
    omci_pm_oper_type_t  operType)
{
    GOS_ERROR_CODE          ret;
    MIB_TABLE_INDEX         tableIndex;
    UINT32                  entrySize;
    UINT32                  entryCnt;
    MIB_TABLE_POTSUNI_T     mibPots;
    UINT16                  chid;
    omci_CCPM_history_data_t* pCcpmCntrs;
    MIB_TABLE_INFO_T       *pTableInfo = NULL;

    pTableInfo = MIB_GetTableInfoPtr(MIB_TABLE_POTSUNI_INDEX);
    if (!pTableInfo) return GOS_OK;

    if (0 == MIB_GetTableCurEntryCount(MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_INDEX)) {
        return GOS_OK;
    }

    tableIndex = MIB_TABLE_POTSUNI_INDEX;
    entrySize = MIB_GetTableEntrySize(tableIndex);
    entryCnt = MIB_GetTableCurEntryCount(tableIndex);

    // allocate memory if it's not created yet
    if (!gpCcpmCntrs)
    {
        gpCcpmCntrs = malloc(entryCnt * sizeof(omci_CCPM_history_data_t));
        if (!gpCcpmCntrs) {
            return GOS_FAIL;
        }
    }

    pCcpmCntrs = gpCcpmCntrs;

    // start collect counters for all ports
    ret = MIB_GetFirst(tableIndex, &mibPots, entrySize);
    while (GOS_OK == ret) {
        omci_get_channel_index_by_pots_uni_me_id(mibPots.EntityId, &chid);

        if (chid < gInfo.devCapabilities.potsPortNum) {
            memset(pCcpmCntrs, 0, sizeof(omci_CCPM_history_data_t));

            // always update PM values
            OMCI_GET_PM_CCPM_LOCK;
            pCcpmCntrs->ch_id = chid;

            VOICE_WRAPPER(omci_voice_CCPM_history_data_get,  pCcpmCntrs);

            OMCI_GET_PM_CCPM_UNLOCK;

            // TBD: The below code should be removed if rtp stat is read_clear.
            if (OMCI_PM_OPER_SWAP == operType ||
                OMCI_PM_OPER_UPDATE_AND_SWAP == operType ||
                OMCI_PM_OPER_RESET == operType)
            {
                // reset PM values
                VOICE_WRAPPER(omci_voice_CCPM_stat_clear, chid);
            }
        }

        pCcpmCntrs++;
        ret = MIB_GetNext(tableIndex, &mibPots, entrySize);
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_pm_getcurrent_ccpm (
    omci_me_instance_t        instanceID,
    omci_CCPM_history_data_t* pCntrs)
{
    GOS_ERROR_CODE             ret;
    MIB_TABLE_INDEX            tableIndex;
    UINT32                     entrySize;
    MIB_TABLE_POTSUNI_T        mibPots;
    omci_CCPM_history_data_t*  pCcpmCntrs;
    MIB_TABLE_INFO_T*          pTableInfo = NULL;

    pTableInfo = MIB_GetTableInfoPtr(MIB_TABLE_POTSUNI_INDEX);
    if (!pTableInfo) {
        return GOS_OK;
    }


    if (!pCntrs) {
        return GOS_FAIL;
    }

    tableIndex = MIB_TABLE_POTSUNI_INDEX;
    entrySize = MIB_GetTableEntrySize(tableIndex);

    pCcpmCntrs = gpCcpmCntrs;
    if (!pCcpmCntrs) {
        return GOS_FAIL;
    }

    // start search counter for specified port
    ret = MIB_GetFirst(tableIndex, &mibPots, entrySize);
    while (GOS_OK == ret) {
        if (mibPots.EntityId != instanceID) {
            goto get_next;
        }

        OMCI_GET_PM_CCPM_LOCK;
        *pCntrs = *pCcpmCntrs;
        OMCI_GET_PM_CCPM_UNLOCK;

        return GOS_OK;

    get_next:
        pCcpmCntrs++;
        ret = MIB_GetNext(tableIndex, &mibPots, entrySize);
    }

    return GOS_ERR_NOT_FOUND;
}


GOS_ERROR_CODE
omci_pm_update_sipAgent(
    omci_pm_oper_type_t  operType)
{
    GOS_ERROR_CODE          ret;
    MIB_TABLE_INDEX         tableIndex;
    UINT32                  entrySize;
    UINT32                  entryCnt;
    MIB_TABLE_SIPAGENTCONFIGDATA_T mibSipAgentCfgData;
    UINT16                  chid;
    omci_SIPAGPM_history_data_t *pSipagpmCntrs;
    MIB_TABLE_INFO_T       *pTableInfo = NULL;

    pTableInfo = MIB_GetTableInfoPtr(MIB_TABLE_SIPAGENTCONFIGDATA_INDEX);
    if (!pTableInfo) {
        OMCI_LOG(
            OMCI_LOG_LEVEL_ERR,
            "Failed to get table for %s\n",
            MIB_GetTableName(MIB_TABLE_SIPAGENTCONFIGDATA_INDEX));
        return GOS_FAIL;
    }

    if (0 == MIB_GetTableCurEntryCount(MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_INDEX))
    {
        OMCI_LOG(
            OMCI_LOG_LEVEL_WARN,
            "Current Entry is empty for %s\n",
            MIB_GetTableName(MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_INDEX));
        return GOS_FAIL;
    }

    tableIndex = MIB_TABLE_SIPAGENTCONFIGDATA_INDEX;
    entrySize = MIB_GetTableEntrySize(tableIndex);
    entryCnt = MIB_GetTableCurEntryCount(tableIndex);

    // allocate memory if it's not created yet
    if (!gpSipagpmCntrs) {
        gpSipagpmCntrs = malloc(entryCnt * sizeof(omci_SIPAGPM_history_data_t));
        if (!gpSipagpmCntrs) {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Failed to allocate storage for gpSipagmCntrs\n");
            return GOS_FAIL;
        }
    }

    pSipagpmCntrs = gpSipagpmCntrs;

    // start collect counters for all ports
    ret = MIB_GetFirst(tableIndex, &mibSipAgentCfgData, entrySize);
    while (GOS_OK == ret) {
        if (mibSipAgentCfgData.EntityId > 0)
            chid = mibSipAgentCfgData.EntityId - 1;
        else
            chid = 0;

        if (chid < gInfo.devCapabilities.potsPortNum) {
            memset(pSipagpmCntrs, 0, sizeof(omci_SIPAGPM_history_data_t));

            // always update PM values
            OMCI_GET_PM_SIPAGPM_LOCK;
            pSipagpmCntrs->ch_id = chid;

            VOICE_WRAPPER(omci_voice_SIPAGPM_history_data_get,  pSipagpmCntrs);

            OMCI_GET_PM_SIPAGPM_UNLOCK;

            // TBD: The below code should be removed if rtp stat is read_clear.
            if (OMCI_PM_OPER_SWAP == operType ||
                OMCI_PM_OPER_UPDATE_AND_SWAP == operType ||
                OMCI_PM_OPER_RESET == operType) {
                // reset PM values
                VOICE_WRAPPER(omci_voice_SIPAGPM_stat_clear, chid);
            }
        }

        pSipagpmCntrs++;
        ret = MIB_GetNext(tableIndex, &mibSipAgentCfgData, entrySize);
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_pm_getcurrent_sipAgent(
    omci_me_instance_t        instanceID,
    omci_SIPAGPM_history_data_t* pCntrs)
{
    GOS_ERROR_CODE             ret;
    MIB_TABLE_INDEX            tableIndex;
    UINT32                     entrySize;
    MIB_TABLE_SIPAGENTCONFIGDATA_T mibSipAgentCfgData;
    omci_SIPAGPM_history_data_t *pSipagpmCntrs;
    MIB_TABLE_INFO_T *pTableInfo = NULL;

    pTableInfo = MIB_GetTableInfoPtr(MIB_TABLE_SIPAGENTCONFIGDATA_INDEX);
    if (!pTableInfo) {
        return GOS_OK;
    }

    if (!pCntrs) {
        return GOS_FAIL;
    }

    tableIndex = MIB_TABLE_SIPAGENTCONFIGDATA_INDEX;
    entrySize = MIB_GetTableEntrySize(tableIndex);

    pSipagpmCntrs = gpSipagpmCntrs;
    if (!pSipagpmCntrs) {
        return GOS_FAIL;
    }

    // start search counter for specified port
    ret = MIB_GetFirst(tableIndex, &mibSipAgentCfgData, entrySize);
    while (GOS_OK == ret) {
        if (mibSipAgentCfgData.EntityId != instanceID) {
            printf(
                "koba %s: EntityId of MibSipAgentCfgDat 0x%x, 0x%0x\n",
                __FUNCTION__,
                mibSipAgentCfgData.EntityId,
                instanceID);
            goto get_next;
        }

        OMCI_GET_PM_SIPAGPM_LOCK;
        *pCntrs = *pSipagpmCntrs;
        OMCI_GET_PM_SIPAGPM_UNLOCK;

        return GOS_OK;

    get_next:
        pSipagpmCntrs++;
        ret = MIB_GetNext(tableIndex, &mibSipAgentCfgData, entrySize);
    }

    return GOS_ERR_NOT_FOUND;
}


GOS_ERROR_CODE
omci_pm_update_sipCi(
    omci_pm_oper_type_t  operType)
{
    GOS_ERROR_CODE          ret;
    MIB_TABLE_INDEX         tableIndex;
    UINT32                  entrySize;
    UINT32                  entryCnt;
    MIB_TABLE_SIPAGENTCONFIGDATA_T mibSipAgentCfgData;
    UINT16                  chid;
    omci_SIPCIPM_history_data_t* pSipcipmCntrs;
    MIB_TABLE_INFO_T       *pTableInfo = NULL;

    pTableInfo = MIB_GetTableInfoPtr(MIB_TABLE_SIPAGENTCONFIGDATA_INDEX);
    if (!pTableInfo) {
        OMCI_LOG(
            OMCI_LOG_LEVEL_ERR,
            "Failed to get table for %s\n",
            MIB_GetTableName(MIB_TABLE_SIPAGENTCONFIGDATA_INDEX));
        return GOS_FAIL;
    }

    if (0 == MIB_GetTableCurEntryCount(MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_INDEX)) {
        OMCI_LOG(
            OMCI_LOG_LEVEL_WARN,
            "Current Entry is empty for %s\n",
            MIB_GetTableName(MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_INDEX));
        return GOS_FAIL;
    }

    tableIndex = MIB_TABLE_SIPAGENTCONFIGDATA_INDEX;
    entrySize = MIB_GetTableEntrySize(tableIndex);
    entryCnt = MIB_GetTableCurEntryCount(tableIndex);

    // allocate memory if it's not created yet
    if (!gpSipcipmCntrs) {
        gpSipcipmCntrs = malloc(entryCnt * sizeof(omci_SIPCIPM_history_data_t));
        if (!gpSipcipmCntrs) {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Failed to allocate storage for gpSipcimCntrs\n");
            return GOS_FAIL;
        }
    }

    pSipcipmCntrs = gpSipcipmCntrs;

    // start collect counters for all ports
    ret = MIB_GetFirst(tableIndex, &mibSipAgentCfgData, entrySize);
    while (GOS_OK == ret) {
        if (mibSipAgentCfgData.EntityId > 0)
            chid = mibSipAgentCfgData.EntityId - 1;
        else
            chid = 0;

        if (chid < gInfo.devCapabilities.potsPortNum) {
            memset(pSipcipmCntrs, 0, sizeof(omci_SIPCIPM_history_data_t));

            // always update PM values
            OMCI_GET_PM_SIPCIPM_LOCK;
            pSipcipmCntrs->ch_id = chid;

            VOICE_WRAPPER(omci_voice_SIPCIPM_history_data_get,  pSipcipmCntrs);

            OMCI_GET_PM_SIPCIPM_UNLOCK;

            // TBD: The below code should be removed if rtp stat is read_clear.
            if (OMCI_PM_OPER_SWAP == operType ||
                OMCI_PM_OPER_UPDATE_AND_SWAP == operType ||
                OMCI_PM_OPER_RESET == operType) {
                // reset PM values
                VOICE_WRAPPER(omci_voice_SIPCIPM_stat_clear, chid);
            }
        }

        pSipcipmCntrs++;
        ret = MIB_GetNext(tableIndex, &mibSipAgentCfgData, entrySize);
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_pm_getcurrent_sipCi(
    omci_me_instance_t           instanceID,
    omci_SIPCIPM_history_data_t* pCntrs)
{
    GOS_ERROR_CODE             ret;
    MIB_TABLE_INDEX            tableIndex;
    UINT32                     entrySize;
    MIB_TABLE_SIPAGENTCONFIGDATA_T mibSipAgentCfgData;
    omci_SIPCIPM_history_data_t *pSipcipmCntrs;
    MIB_TABLE_INFO_T *pTableInfo = NULL;

    pTableInfo = MIB_GetTableInfoPtr(MIB_TABLE_SIPAGENTCONFIGDATA_INDEX);
    if (!pTableInfo) {
        return GOS_OK;
    }

    if (!pCntrs) {
        return GOS_FAIL;
    }

    tableIndex = MIB_TABLE_SIPAGENTCONFIGDATA_INDEX;
    entrySize = MIB_GetTableEntrySize(tableIndex);

    pSipcipmCntrs = gpSipcipmCntrs;
    if (!pSipcipmCntrs) {
        return GOS_FAIL;
    }

    // start search counter for specified port
    ret = MIB_GetFirst(tableIndex, &mibSipAgentCfgData, entrySize);
    while (GOS_OK == ret) {
        if (mibSipAgentCfgData.EntityId != instanceID) {
            goto get_next;
        }

        OMCI_GET_PM_SIPCIPM_LOCK;
        *pCntrs = *pSipcipmCntrs;
        OMCI_GET_PM_SIPCIPM_UNLOCK;

        return GOS_OK;

    get_next:
        pSipcipmCntrs++;
        ret = MIB_GetNext(tableIndex, &mibSipAgentCfgData, entrySize);
    }

    return GOS_ERR_NOT_FOUND;
}

GOS_ERROR_CODE omci_pm_release_all_counters(void)
{
    free(gpEthPortCntrs);
    free(gpGemPortCntrs);
    gGemPortEntryCnt = 0;

    return GOS_OK;
}

GOS_ERROR_CODE omci_pm_is_threshold_crossed(MIB_TABLE_INDEX     tableIndex,
                                            omci_me_instance_t  instanceID,
                                            void                *pRow,
                                            UINT8               *pTcaAlmNumber,
                                            UINT8               *pTcaAttrMap,
                                            UINT16              *pTcaDisableMask,
                                            BOOL                *pIsTcaRaised)
{
    MIB_TABLE_T                 *pTable;
    mib_alarm_table_t           alarmTable;
    omci_me_instance_t          thresholdID;
    MIB_TABLE_THRESHOLDDATA1_T  mibThresholdData1;
    MIB_TABLE_THRESHOLDDATA2_T  mibThresholdData2;
    UINT8                       i, j;
    UINT32                      attrThreshold;
    UINT8                       *pAttrPtr;
    unsigned long long          attrValue;

    if (!pRow || !pTcaAlmNumber || !pTcaAttrMap || !pIsTcaRaised)
        return GOS_ERR_PARAM;

    pTable = mib_GetTablePtr(tableIndex);
    if (!pTable)
        return GOS_ERR_PARAM;

    *pIsTcaRaised = FALSE;

    // get alarm table
    if (GOS_OK != mib_alarm_table_get(tableIndex, instanceID, &alarmTable))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Get alarm table fail: %s, 0x%x",
            MIB_GetTableName(tableIndex), instanceID);

        return GOS_FAIL;
    }

    // get threshold id
    pAttrPtr = mib_GetAttrPtr(tableIndex, pRow, MIB_ATTR_FIRST_INDEX + 2);
    memcpy(&thresholdID, pAttrPtr, sizeof(omci_me_instance_t));

    // get threshold data 1
    memset(&mibThresholdData1, 0, sizeof(MIB_TABLE_THRESHOLDDATA1_T));
    mibThresholdData1.EntityId = thresholdID;
    if (GOS_OK != MIB_Get(MIB_TABLE_THRESHOLDDATA1_INDEX,
            &mibThresholdData1, sizeof(MIB_TABLE_THRESHOLDDATA1_T)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Instance not found: %s, 0x%x",
            MIB_GetTableName(MIB_TABLE_THRESHOLDDATA1_INDEX), thresholdID);

        return GOS_ERR_NOT_FOUND;
    }

    // get threshold data 2 if necessary
    if ((MIB_GetTableAttrNum(tableIndex) - 3) > 7)
    {
        memset(&mibThresholdData2, 0, sizeof(MIB_TABLE_THRESHOLDDATA2_T));
        mibThresholdData2.EntityId = thresholdID;
        if (GOS_OK != MIB_Get(MIB_TABLE_THRESHOLDDATA2_INDEX,
                &mibThresholdData2, sizeof(MIB_TABLE_THRESHOLDDATA2_T)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Instance not found: %s, 0x%x",
                MIB_GetTableName(MIB_TABLE_THRESHOLDDATA2_INDEX), thresholdID);

            return GOS_ERR_NOT_FOUND;
        }
    }

    // loop attributes to check for TCA, control blocks are skipped
    for (i = 3; i < MIB_GetTableAttrNum(tableIndex); i++)
    {
        // skip G.988 TCA don't care attributes
        if (0 == pTcaAttrMap[i])
            continue;

        // for attributes number > 7, threshold data 2 will be used
        if (pTcaAttrMap[i] > 7)
        {
            MIB_GetAttrFromBuf(MIB_TABLE_THRESHOLDDATA2_INDEX,
                pTcaAttrMap[i] - 7 + 1, &attrThreshold, &mibThresholdData2, sizeof(attrThreshold));
        }
        // otherwise, threshold data 1 will be used
        else
        {
            MIB_GetAttrFromBuf(MIB_TABLE_THRESHOLDDATA1_INDEX,
                pTcaAttrMap[i] + 1, &attrThreshold, &mibThresholdData1, sizeof(attrThreshold));
        }

        // skip G.988 TCA not enable attributes
        if (attrThreshold != 0 && attrThreshold != UINT_MAX)
        {
            pAttrPtr = mib_GetAttrPtr(tableIndex, pRow, i + 1);
            attrValue = 0;

            // copy data
            for (j = 0; j < MIB_GetAttrSize(tableIndex, i + 1); j++)
            {
                attrValue = attrValue << 8;
                attrValue += pAttrPtr[j];
            }

            // continue if not crossing
            if (attrValue >= attrThreshold)
            {
                // raise TCA if it has not been raised yet
                if (OMCI_ALM_STS_DECLARE != m_omci_get_alm_sts(alarmTable.aBitMask, pTcaAlmNumber[i]))
                {
                    alarmTable.aBitMask[m_omci_alm_nbr_in_byte(pTcaAlmNumber[i])] |=
                        m_omci_alm_nbr_in_bit(pTcaAlmNumber[i]);

                    // check tca disable mask before really raising the tca flag
                    if (!pTcaDisableMask || (!m_omci_ext_pm_tca_all_disable(*pTcaDisableMask) &&
                            !m_omci_ext_pm_tca_disable(*pTcaDisableMask, i + 1)))
                        *pIsTcaRaised = TRUE;
                }
            }
            else
            {
                // cancel TCA if it has been raised
                if (OMCI_ALM_STS_DECLARE == m_omci_get_alm_sts(alarmTable.aBitMask, pTcaAlmNumber[i]))
                {
                    alarmTable.aBitMask[m_omci_alm_nbr_in_byte(pTcaAlmNumber[i])] &=
                        ~m_omci_alm_nbr_in_bit(pTcaAlmNumber[i]);
                }
            }
        }
    }

    // set alarm table no matter TCA raised or not
    if (GOS_OK != mib_alarm_table_set(tableIndex, instanceID, &alarmTable))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set alarm table fail: %s, 0x%x",
            MIB_GetTableName(tableIndex), instanceID);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE omci_pm_clear_all_raised_tca(MIB_TABLE_INDEX     tableIndex,
                                            omci_me_instance_t  instanceID)
{
    mib_alarm_table_t   alarmTable;
    mib_alarm_table_t   zeroAlarmTable;

    memset(&zeroAlarmTable, 0, sizeof(mib_alarm_table_t));

    // get alarm table
    if (GOS_OK != mib_alarm_table_get(tableIndex, instanceID, &alarmTable))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Get alarm table fail: %s, 0x%x",
            MIB_GetTableName(tableIndex), instanceID);

        return GOS_FAIL;
    }

    if (0 == memcmp(&alarmTable, &zeroAlarmTable, sizeof(mib_alarm_table_t)))
        return GOS_OK;

    // set alarm table
    if (GOS_OK !=
            mib_alarm_table_set(tableIndex, instanceID, &zeroAlarmTable))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set alarm table fail: %s, 0x%x",
            MIB_GetTableName(tableIndex), instanceID);

        return GOS_FAIL;
    }

    // send TCA for cancelling
    if (GOS_OK !=
            omci_alarm_notify_xmit(tableIndex, instanceID, &zeroAlarmTable))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Send alarm notify fail: %s, 0x%x",
            MIB_GetTableName(tableIndex), instanceID);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE omci_pm_clear_raised_tca(MIB_TABLE_INDEX     tableIndex,
                                        omci_me_instance_t  instanceID,
                                        UINT8               *pTcaAlmNumber,
                                        UINT8               *pTcaAttrMap,
                                        UINT16              *pTcaDisableMask)
{
    mib_alarm_table_t   alarmTable;
    mib_alarm_table_t   zeroAlarmTable;
    UINT8               i;

    if (!pTcaAlmNumber || !pTcaAttrMap || !pTcaDisableMask)
        return GOS_ERR_PARAM;

    memset(&zeroAlarmTable, 0, sizeof(mib_alarm_table_t));

    // get alarm table
    if (GOS_OK != mib_alarm_table_get(tableIndex, instanceID, &alarmTable))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Get alarm table fail: %s, 0x%x",
            MIB_GetTableName(tableIndex), instanceID);

        return GOS_FAIL;
    }

    if (0 == memcmp(&alarmTable, &zeroAlarmTable, sizeof(mib_alarm_table_t)))
        return GOS_OK;

    // loop attributes to check for TCA, control blocks are skipped
    for (i = 3; i < MIB_GetTableAttrNum(tableIndex); i++)
    {
        // skip G.988 TCA don't care attributes
        if (0 == pTcaAttrMap[i])
            continue;

        // clear all disabled tca if it has been raised
        if (m_omci_ext_pm_tca_disable(*pTcaDisableMask, i + 1))
        {
            if (OMCI_ALM_STS_DECLARE == m_omci_get_alm_sts(alarmTable.aBitMask, pTcaAlmNumber[i]))
            {
                alarmTable.aBitMask[m_omci_alm_nbr_in_byte(pTcaAlmNumber[i])] &=
                    ~m_omci_alm_nbr_in_bit(pTcaAlmNumber[i]);
            }
        }
    }

    // set alarm table
    if (GOS_OK !=
            mib_alarm_table_set(tableIndex, instanceID, &alarmTable))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set alarm table fail: %s, 0x%x",
            MIB_GetTableName(tableIndex), instanceID);

        return GOS_FAIL;
    }

    // send TCA for cancelling
    if (GOS_OK !=
            omci_alarm_notify_xmit(tableIndex, instanceID, &alarmTable))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Send alarm notify fail: %s, 0x%x",
            MIB_GetTableName(tableIndex), instanceID);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE omci_is_traffic_descriptor_supported(MIB_TABLE_TRAFFICDESCRIPTOR_T   *pMibTD)
{
    if (!pMibTD)
        return GOS_ERR_PARAM;

    // only support colour blind mode
    if (TD_COLOUR_MODE_COLOUR_BLIND != pMibTD->ColourMode)
        return GOS_ERR_NOTSUPPORT;

    // ingress colour marking is not meaningful in colour blind mode

    // not support egress colour makring of packet modification
    if (TD_EGRESS_COLOUR_NO_MARKING != pMibTD->EgressColourMarking &&
            TD_EGRESS_COLOUR_INTERNAL_MARKING != pMibTD->EgressColourMarking)
        return GOS_ERR_NOTSUPPORT;

    return GOS_OK;
}

GOS_ERROR_CODE omci_is_traffic_descriptor_existed(omci_me_instance_t                instanceID,
                                                    MIB_TABLE_TRAFFICDESCRIPTOR_T   *pMibTD)
{
    if (!pMibTD)
        return GOS_ERR_PARAM;

    // search traffic descriptor instance
    pMibTD->EntityId = instanceID;
    if (GOS_OK != MIB_Get(MIB_TABLE_TRAFFICDESCRIPTOR_INDEX, pMibTD, sizeof(MIB_TABLE_TRAFFICDESCRIPTOR_T)))
        return GOS_ERR_NOT_FOUND;

    return omci_is_traffic_descriptor_supported(pMibTD);
}

GOS_ERROR_CODE omci_apply_traffic_descriptor_to_gem_flow(OMCI_GEM_FLOW_ts               flowCfg,
                                                        MIB_TABLE_TRAFFICDESCRIPTOR_T   *pMibTD)
{
    if (pMibTD)
    {
        flowCfg.cir = pMibTD->CIR;
        feature_api(FEATURE_API_ME_00080000_FORCE_METERTYPE, &pMibTD->MeterType);
        flowCfg.pir = (TD_METER_TYPE_RFC_4115 == pMibTD->MeterType) ?
            (pMibTD->PIR - pMibTD->CIR) : pMibTD->PIR;
    }
    else
    {
        flowCfg.cir = 0;
        flowCfg.pir = 0;
    }

    // set gem flow rate cfg
    return omci_wrapper_setGemFlowRate(flowCfg);
}

GOS_ERROR_CODE omci_apply_traffic_descriptor_to_gem_port(omci_me_instance_t             instanceID,
                                                        PON_GEMPORT_DIRECTION           gemDirection,
                                                        MIB_TABLE_TRAFFICDESCRIPTOR_T   *pMibTD)

{
    MIB_TABLE_GEMPORTCTP_T  mibGPNC;
    OMCI_GEM_FLOW_ts        flowCfg;

    // search gem port network ctp instance
    mibGPNC.EntityID = instanceID;
    if (GOS_OK != MIB_Get(MIB_TABLE_GEMPORTCTP_INDEX, &mibGPNC, sizeof(MIB_TABLE_GEMPORTCTP_T)))
        return GOS_ERR_NOT_FOUND;

    flowCfg.dir = gemDirection;
    flowCfg.portId = mibGPNC.PortID;

    return omci_apply_traffic_descriptor_to_gem_flow(flowCfg, pMibTD);
}


GOS_ERROR_CODE omci_bridgePortIndexGet(UINT16 entityId, UINT32 *pPortIdx)
{
    MIB_TABLE_MACBRIPORTCFGDATA_T tmpMacBriPortCfgData, *pMibMacBriPortCfgData;

    tmpMacBriPortCfgData.EntityID = entityId;

    if(!mib_FindEntry(MIB_TABLE_MACBRIPORTCFGDATA_INDEX,&tmpMacBriPortCfgData,&pMibMacBriPortCfgData))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "can't find MacBriPortCfgData 0x%x\n", entityId);
        return GOS_OK;
    }

    /*UNI side*/
    if(MBPCD_TP_TYPE_PPTP_ETH_UNI == pMibMacBriPortCfgData->TPType)
    {
        /*Get port index form entity id*/
        if (GOS_OK != pptp_eth_uni_me_id_to_switch_port(pMibMacBriPortCfgData->TPPointer, (UINT16 *)pPortIdx))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "PPTP instance ID translate error: 0x%x", pMibMacBriPortCfgData->TPPointer);
            return GOS_FAIL;
        }
    }
    /*ANI side*/
    else if((MBPCD_TP_TYPE_GEM_IWTP == pMibMacBriPortCfgData->TPType) || (MBPCD_TP_TYPE_IEEE_8021P_MAPPER == pMibMacBriPortCfgData->TPType))
    {
        *pPortIdx = gInfo.devCapabilities.ponPort;
    }
    return GOS_OK;
}

GOS_ERROR_CODE
omci_mngCapOfUnigGet (
    UINT16             entityId,
    unig_attr_mgmt_capability_t* mngCap
)
{
    GOS_ERROR_CODE rt = GOS_FAIL;
    MIB_TABLE_UNIG_T mibUnig;

    if (!mngCap)
    {
        printf("Storage of mngCap is NULL\n");
        rt = GOS_ERR_INVALID_INPUT;
        goto OMCI_MNGCAPOFUNIGGET_ERROR;
    }

    //
    // Check whether ManageCapibility of UNI-G is UNIG_MGMT_CAPABILITY_NON_OMCI_ONLY
    //
    mibUnig.EntityID = entityId;
    mibUnig.ManageCapability = UNIG_MGMT_CAPABILITY_BOTH_OMCI_NON_OMCI;

    if (GOS_OK != MIB_Get(MIB_TABLE_UNIG_INDEX, &mibUnig, sizeof (MIB_TABLE_UNIG_T)))
    {
        OMCI_LOG(
            OMCI_LOG_LEVEL_ERR,
            "%s: Failed to get UNIG(0x%03X) for entity (0x%03X)",
            __FUNCTION__,
            mibUnig.EntityID,
            entityId);

        rt = GOS_FAIL;
    }
    else
    {
        rt = GOS_OK;
    }

    *mngCap = mibUnig.ManageCapability;

OMCI_MNGCAPOFUNIGGET_ERROR:
    return rt;
}

GOS_ERROR_CODE anig_generic_transceiver_para_updater(double     *pOutput,
                            omci_transceiver_status_type_t      type)
{
    GOS_ERROR_CODE              ret = GOS_OK;
    omci_transceiver_status_t   trancvrStatus;

    if (!pOutput)
        return GOS_ERR_PARAM;

   switch (type)
    {
        case OMCI_TRANSCEIVER_STATUS_TYPE_TEMPERATURE:
        case OMCI_TRANSCEIVER_STATUS_TYPE_VOLTAGE:
        case OMCI_TRANSCEIVER_STATUS_TYPE_BIAS_CURRENT:
        case OMCI_TRANSCEIVER_STATUS_TYPE_TX_POWER:
        case OMCI_TRANSCEIVER_STATUS_TYPE_RX_POWER:
            break;
        default:
            return GOS_ERR_NOTSUPPORT;
            break;
    }

    trancvrStatus.type = type;

    ret = omci_wrapper_getTransceiverStatus(&trancvrStatus);
    if (GOS_OK != ret)
        return GOS_FAIL;

    switch (type)
    {
        case OMCI_TRANSCEIVER_STATUS_TYPE_TEMPERATURE:
            // unit: degrees (C)
            if (128 >= trancvrStatus.data[0])
                *pOutput = (-1)*((~(trancvrStatus.data[0]))+1)+((double)(trancvrStatus.data[1])*1/256);
            else
                *pOutput = trancvrStatus.data[0]+((double)(trancvrStatus.data[1])*1/256);
            break;
        case OMCI_TRANSCEIVER_STATUS_TYPE_VOLTAGE:
            // unit: 100 uV
            *pOutput = (double)((trancvrStatus.data[0] << 8) | trancvrStatus.data[1]);
            break;
        case OMCI_TRANSCEIVER_STATUS_TYPE_BIAS_CURRENT:
            // unit: 2 uA
            *pOutput = (double)((trancvrStatus.data[0] << 8) | trancvrStatus.data[1]);
            break;
        case OMCI_TRANSCEIVER_STATUS_TYPE_TX_POWER:
        case OMCI_TRANSCEIVER_STATUS_TYPE_RX_POWER:
            // unit: 0.1 uW -> dBm
            *pOutput = omci_util_log10((double)((trancvrStatus.data[0] << 8) | trancvrStatus.data[1])*1/10000)*10;
            break;
        default:
            break;
    }

    return ret;
}

void omci_GetPrivateVlanCfgObj(MIB_TABLE_PRIVATE_VLANCFG_T **ppMibPrivateVlanCfg)
{
    MIB_TABLE_PRIVATE_VLANCFG_T privateVlanCfg;
    privateVlanCfg.EntityID = 0;
    if(!mib_FindEntry(MIB_TABLE_PRIVATE_VLANCFG_INDEX, &privateVlanCfg, ppMibPrivateVlanCfg))
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "mib_FindEntry MIB_TABLE_PRIVATE_VLANCFG_INDEX failed...%s", __FUNCTION__);

    return;
}

GOS_ERROR_CODE
OMCI_IotVlanCfgSet_Cmd(unsigned char type, unsigned char mode, unsigned char pri, unsigned short vid)
{
    MIB_TABLE_PRIVATE_VLANCFG_T privateVlanCfg, oldPrivateVlanCfg;
    MIB_ATTRS_SET mibAttrsSet;

    gInfo.iotVlanCfg.vlan_cfg_type = type;
    gInfo.iotVlanCfg.vlan_cfg_manual_mode = mode;
    gInfo.iotVlanCfg.vlan_cfg_manual_pri = pri;
    gInfo.iotVlanCfg.vlan_cfg_manual_vid = vid;

    privateVlanCfg.EntityID = 0;
    if (GOS_OK == MIB_Get(MIB_TABLE_PRIVATE_VLANCFG_INDEX, &privateVlanCfg, sizeof(MIB_TABLE_PRIVATE_VLANCFG_T)))
    {
        memcpy(&oldPrivateVlanCfg, &privateVlanCfg, sizeof(MIB_TABLE_PRIVATE_VLANCFG_T));
        privateVlanCfg.Type = gInfo.iotVlanCfg.vlan_cfg_type;
        privateVlanCfg.ManualMode = gInfo.iotVlanCfg.vlan_cfg_manual_mode;
        privateVlanCfg.ManualTagVid = gInfo.iotVlanCfg.vlan_cfg_manual_vid;
        privateVlanCfg.ManualTagPri = gInfo.iotVlanCfg.vlan_cfg_manual_pri;
        GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_PRIVATE_VLANCFG_INDEX, &privateVlanCfg,
            sizeof(MIB_TABLE_PRIVATE_VLANCFG_T)));
    }

    MIB_ClearAttrSet(&mibAttrsSet);

    MIB_SetAttrSet(&mibAttrsSet, MIB_TABLE_PRIVATE_VLANCFG_TYPE);
    MIB_SetAttrSet(&mibAttrsSet, MIB_TABLE_PRIVATE_VLANCFG_MANUAL_MODE);
    MIB_SetAttrSet(&mibAttrsSet, MIB_TABLE_PRIVATE_VLANCFG_MANUAL_TAG_VID);
    MIB_SetAttrSet(&mibAttrsSet, MIB_TABLE_PRIVATE_VLANCFG_MANUAL_TAG_PRI);

    if (GOS_OK != OMCI_MeOperCfg(MIB_TABLE_PRIVATE_VLANCFG_INDEX,
            &oldPrivateVlanCfg, &privateVlanCfg, MIB_SET, mibAttrsSet, OMCI_MSG_BASELINE_PRI_LOW))
    {
        OMCI_PRINT("OMCI_IotVlanCfgSet private vlan cfg OMCI_MeOperCfg error");
        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE OMCI_IotVlanDetect_Cmd(void)
{
    omci_generate_vlan_info();
    return GOS_OK;
}

GOS_ERROR_CODE OMCI_DotGen_Cmd(void)
{
    omci_generate_dot_file();
    return GOS_OK;
}

GOS_ERROR_CODE OMCI_RegModShow_Cmd(void)
{
    int oldstdout = -1, nul = -1;
    const int stdout_fd = fileno(stdout);
    nul = omci_open_cli_fd();

    if (-1 != nul)
    {
        oldstdout = dup(stdout_fd);
        dup2(nul, stdout_fd);
        close(nul);
    }

    feature_reg_module_show();

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

GOS_ERROR_CODE OMCI_RegApiShow_Cmd(void)
{
    int oldstdout = -1, nul = -1;
    const int stdout_fd = fileno(stdout);
    nul = omci_open_cli_fd();

    if (-1 != nul)
    {
        oldstdout = dup(stdout_fd);
        dup2(nul, stdout_fd);
        close(nul);
    }

    feature_reg_api_show();
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

GOS_ERROR_CODE OMCI_cFlagSet_Cmd(char *type, unsigned int level)
{
    if (!strcmp(type, "bdp"))
    {
        /* notify FAL */
        fMgmtInit_internal(gInfo.customize.bridgeDP, level, 0);
        gInfo.customize.bridgeDP = level;
    }
    else if (!strcmp(type, "rdp"))
    {
        /* notify FAL */
        fMgmtInit_internal(gInfo.customize.routeDP, level, 1);
        gInfo.customize.routeDP = level;
    }
    else if (!strcmp(type, "multicast"))
    {
        /* notify FAL */
        fMgmtInit_internal(gInfo.customize.multicast, level, 2);
        gInfo.customize.multicast = level;
    }
    else if (!strcmp(type, "me"))
    {
        /* notify FAL */
        fMgmtInit_internal(gInfo.customize.me, level, 3);
        gInfo.customize.me = level;
    }

    return GOS_OK;
}

GOS_ERROR_CODE OMCI_cFlagGet_Cmd(void)
{
    int oldstdout = -1, nul = -1;
    const int stdout_fd = fileno(stdout);
    nul = omci_open_cli_fd();

    if (-1 != nul)
    {
        oldstdout = dup(stdout_fd);
        dup2(nul, stdout_fd);
        close(nul);
    }

    OMCI_PRINT("BDP: 0x%08x", gInfo.customize.bridgeDP);
    OMCI_PRINT("RDP: 0x%08x", gInfo.customize.routeDP);
    OMCI_PRINT(" MC: 0x%08x", gInfo.customize.multicast);
    OMCI_PRINT(" ME: 0x%08x", gInfo.customize.me);

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

GOS_ERROR_CODE omci_apply_traffic_descriptor_to_uni_port(
    UINT16  ethUniID, OMCI_UNI_RATE_DIRECTION dir, MIB_TABLE_TRAFFICDESCRIPTOR_T *pMibTD)
{
    UINT16 portId;
    omci_port_rate_t ratePerPort;

    memset(&ratePerPort, 0, sizeof(omci_port_rate_t));

    if (GOS_OK == pptp_eth_uni_me_id_to_switch_port(ethUniID, &portId))
    {
        ratePerPort.port = portId;
        ratePerPort.dir = dir;
        if (pMibTD) {
            ratePerPort.rate = (pMibTD->PIR) * 8 / 1024;
            feature_api(FEATURE_API_ME_00080000, pMibTD, &ratePerPort);
        } else {
            ratePerPort.rate = MAX_BW_RATE;
        }
        return omci_wrapper_setUniPortRate(&ratePerPort);
    }
    return GOS_OK;
}

GOS_ERROR_CODE OMCI_MibCreate_Cmd(int classId, UINT16 entityId, char *value)
{
    MIB_TABLE_INDEX         tableIndex;
    MIB_ATTR_INDEX          attrIndex;
    MIB_ATTR_TYPE           attrType;
    UINT32                  entrySize;
    UINT32                  attrSize;
    omci_msg_attr_mask_t    attrMask;
    MIB_ATTRS_SET           attrSet;
    UINT8                   mibBuf[MIB_TABLE_ENTRY_MAX_SIZE];
    UINT8                   *pAttrBuf;
    char                    *pValue;
    char                    byteBuf[3];
    char                    *pByte;
    UINT8                   i, j;

    if (!value)
        return GOS_ERR_PARAM;

    tableIndex = MIB_GetTableIndexByClassId(classId);
    entrySize = MIB_GetTableEntrySize(tableIndex);

    // make sure the ME is not existed yet
    MIB_SetAttrToBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &entityId, mibBuf, sizeof(omci_me_instance_t));

    if (GOS_OK == MIB_Get(tableIndex, mibBuf, entrySize))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,
            "%s: Instance exist, class=%d, entity=0x%x", __FUNCTION__, classId, entityId);

        return GOS_ERR_DUPLICATED;
    }

    // set default attribute value
    MIB_Default(tableIndex, mibBuf, entrySize);
    MIB_SetAttrToBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &entityId, mibBuf, sizeof(omci_me_instance_t));

    // start tokenize
    pValue = strtok(value, " ");

    for (attrIndex = MIB_ATTR_FIRST_INDEX + 1, i = 1; i < MIB_GetTableAttrNum(tableIndex);
            attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex), i++)
    {
        // skip attributes that is not set-by-create
        if (!(MIB_GetAttrOltAcc(tableIndex, attrIndex) & OMCI_ME_ATTR_ACCESS_SBC))
            continue;

        if (pValue)
        {
            attrType = MIB_GetAttrDataType(tableIndex, attrIndex);
            attrSize = MIB_GetAttrSize(tableIndex, attrIndex);

            // set attribute value
            switch (attrType)
            {
                case MIB_ATTR_TYPE_UINT8:
                case MIB_ATTR_TYPE_UINT16:
                case MIB_ATTR_TYPE_UINT32:
                case MIB_ATTR_TYPE_UINT64:
                case MIB_ATTR_TYPE_TABLE:
                    pAttrBuf = malloc(sizeof(UINT8) * attrSize);
                    if (!pAttrBuf)
                        return GOS_FAIL;

                    pByte = pValue;
                    for (j = 0; ; j++)
                    {
                        if (j >= attrSize || j >= (ceil((double)strlen(pValue)/2)))
                            break;

                        snprintf(byteBuf, 3, "%c%c", *pByte, *(pByte + 1));
                        *(pAttrBuf + j) = strtoul(byteBuf, NULL, 16);
                        pByte += 2;
                    }

                    MIB_SetAttrToBuf(tableIndex, attrIndex, pAttrBuf, mibBuf, attrSize);
                    free(pAttrBuf);
                    break;

                case MIB_ATTR_TYPE_STR:
                    MIB_SetAttrToBuf(tableIndex, attrIndex, value, mibBuf, attrSize+1);
                    break;

                default:
                    break;
            }
        }

        // find next token
        pValue = strtok(NULL, " ");
    }

    // create ME
    if (GOS_OK != MIB_Set(tableIndex, mibBuf, entrySize))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,
            "%s: MIB_Set error, class=%d, entity=0x%x", __FUNCTION__, classId, entityId);

        return GOS_FAIL;
    }

    // make sure the ME is created successfully
    if (GOS_OK != MIB_Get(tableIndex, mibBuf, entrySize))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,
            "%s: Instance not exist, class=%d, entity=0x%x", __FUNCTION__, classId, entityId);

        return GOS_ERR_NOT_FOUND;
    }

    // toggle sbc attributes only
    attrMask = omci_GetOltAccAttrSet(tableIndex, OMCI_ME_ATTR_ACCESS_SBC);
    omci_AttrsOmciSetToMibSet(&attrMask, &attrSet);

    // invoke ME operation callback
    if (GOS_OK != OMCI_MeOperCfg(tableIndex, mibBuf, mibBuf, MIB_ADD, attrSet, OMCI_MSG_BASELINE_PRI_LOW))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,
            "%s: OMCI_MeOperCfg error, class=%d, entity=0x%x", __FUNCTION__, classId, entityId);

        MIB_Delete(tableIndex, mibBuf, entrySize);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE OMCI_MibDelete_Cmd(int classId, UINT16 entityId)
{
    MIB_TABLE_INDEX         tableIndex;
    UINT32                  entrySize;
    UINT8                   mibBuf[MIB_TABLE_ENTRY_MAX_SIZE];

    tableIndex = MIB_GetTableIndexByClassId(classId);
    entrySize = MIB_GetTableEntrySize(tableIndex);

    // make sure the ME is existed
    MIB_SetAttrToBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &entityId, mibBuf, sizeof(omci_me_instance_t));

    if (GOS_OK != MIB_Get(tableIndex, mibBuf, entrySize))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,
            "%s: Instance not exist, class=%d, entity=0x%x", __FUNCTION__, classId, entityId);

        return GOS_ERR_NOT_FOUND;
    }

    // invoke ME operation callback
    OMCI_MeOperCfg(tableIndex, mibBuf, mibBuf, MIB_DEL, 0, OMCI_MSG_BASELINE_PRI_LOW);

    // delete ME
    if (GOS_OK != MIB_Delete(tableIndex, mibBuf, entrySize))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,
            "%s: MIB_Delete error, class=%d, entity=0x%x", __FUNCTION__, classId, entityId);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE OMCI_SimAVC_Cmd(int classId, UINT16 entityId, unsigned char attrIndex)
{
    MIB_TABLE_INDEX             tableIndex;
    MIB_ATTRS_SET               avcAttrSet;
    omci_msg_attr_mask_t        avcAttrMask;
    omci_msg_norm_baseline_t    omciMsg;

    tableIndex = MIB_GetTableIndexByClassId(classId);

    if (attrIndex < MIB_ATTR_FIRST_INDEX || attrIndex >= MIB_GetTableAttrNum(tableIndex))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,
            "%s: attribute index out of range, class=%d, entity=0x%x", __FUNCTION__, classId, entityId);

        return GOS_ERR_PARAM;
    }

    memset(&omciMsg, 0, sizeof(omciMsg));
    omciMsg.meId.meClass = classId;
    omciMsg.meId.meInstance = entityId;
    omciMsg.type = OMCI_MSG_TYPE_ATTR_VALUE_CHANGE;

    MIB_ClearAttrSet(&avcAttrSet);
    MIB_SetAttrSet(&avcAttrSet, MIB_ATTR_FIRST_INDEX + attrIndex);
    OMCI_AttrsMibSetToOmciSet(&avcAttrSet, &avcAttrMask);
    GOS_SetUINT16((UINT16*)&omciMsg.content[0], GOS_Htons((UINT16)avcAttrMask));

    return (OMCI_SendIndicationToOlt(&omciMsg, 0) > 0) ? GOS_OK : GOS_FAIL;
}

UINT16 omci_adjust_tbl_ctrl_by_omcc_ver(UINT16 val)
{
    MIB_TABLE_ONT2G_T   mibOnu2G;

    mibOnu2G.EntityID = TXC_ONU2G_INSTANCE_ID;

    // read out the entry data
    if (GOS_OK != MIB_Get(MIB_TABLE_ONT2G_INDEX, &mibOnu2G, sizeof(mibOnu2G)))
        return val;

    if (ONU2G_OMCC_VERSION_0xA1 > mibOnu2G.OMCCVer)
        return (val & 0xC7FF);

    return val;
}

static void omci_pwr_shedding_pre_shed_handler(UINT16   classID,
                                                UINT16  instanceID,
                                                UINT32  privData)
{
    GOS_ERROR_CODE                  ret;
    MIB_TABLE_INDEX                 tableIndex;
    UINT32                          entrySize;
    MIB_TABLE_ONU_PWR_SHEDDING_T    mibOps;
    MIB_TABLE_ONU_PWR_SHEDDING_T    *pMibOps;
    MIB_TABLE_ETHUNI_T              mibPptpEthUNI;
    omci_port_pwr_down_t            portPwrDown;
    UINT16                          portId;

    // get power shedding data
    tableIndex = MIB_TABLE_ONU_PWR_SHEDDING_INDEX;
    mibOps.EntityId = 0;

    if (!mib_FindEntry(tableIndex, &mibOps, &pMibOps))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "cannot find onu power shedding mib entry");

        return;
    }

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,
        "power pre-shedding timer %hu timeout", instanceID);

    // delete timer
    omci_timer_delete_by_id(classID, instanceID);

    if (OMCI_TIMER_PWR_PRE_SHED_DATA_INST_ID == instanceID)
    {
        // start power shedding
        if (!(pMibOps->SheddingStatus & OPS_SHEDDING_STATUS_DATA_CLASS))
        {
            tableIndex = MIB_TABLE_ETHUNI_INDEX;
            entrySize = MIB_GetTableEntrySize(tableIndex);
            portPwrDown.state = TRUE;

            ret = MIB_GetFirst(tableIndex, &mibPptpEthUNI, entrySize);
            while (GOS_OK == ret)
            {
                ret = pptp_eth_uni_me_id_to_switch_port(mibPptpEthUNI.EntityID, &portId);
                if (GOS_OK == ret)
                {
                    portPwrDown.port = portId;

                    ret = omci_wrapper_setPhyPwrDown(&portPwrDown);

                    if (GOS_OK != ret)
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                            "start shedding power for data port %hu fail", portId);
                    }
                    else
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_WARN,
                            "start shedding power for data port %hu", portId);
                    }
                }

                ret = MIB_GetNext(tableIndex, &mibPptpEthUNI, entrySize);
            }

            // update shedding status
            pMibOps->SheddingStatus |= OPS_SHEDDING_STATUS_DATA_CLASS;
        }

        // clear pre-shed timer
        pMibOps->DataShedIntvlRemained = OPS_SHEDDING_INTVL_IMMEDIATE_SHEDDING;
        // reset post-shed timer
        pMibOps->DataResetIntvlRemained = pMibOps->RestorePwrIntvl;
    }

    if (OMCI_TIMER_PWR_PRE_SHED_VOICE_INST_ID == instanceID)
    {
        // start power shedding
        if (!(pMibOps->SheddingStatus & OPS_SHEDDING_STATUS_VOICE_CLASS))
        {
            // TBD

            // update shedding status
            pMibOps->SheddingStatus |= OPS_SHEDDING_STATUS_VOICE_CLASS;
        }

        // clear pre-shed timer
        pMibOps->VoiceShedIntvlRemained = OPS_SHEDDING_INTVL_IMMEDIATE_SHEDDING;
        // reset post-shed timer
        pMibOps->VoiceResetIntvlRemained = pMibOps->RestorePwrIntvl;
    }
}

static void omci_pwr_shedding_post_shed_handler(UINT16  classID,
                                                UINT16  instanceID,
                                                UINT32  privData)
{
    GOS_ERROR_CODE                  ret;
    MIB_TABLE_INDEX                 tableIndex;
    UINT32                          entrySize;
    MIB_TABLE_ONU_PWR_SHEDDING_T    mibOps;
    MIB_TABLE_ONU_PWR_SHEDDING_T    *pMibOps;
    MIB_TABLE_ETHUNI_T              mibPptpEthUNI;
    omci_port_pwr_down_t            portPwrDown;
    UINT16                          portId;

    // get power shedding data
    tableIndex = MIB_TABLE_ONU_PWR_SHEDDING_INDEX;
    mibOps.EntityId = 0;

    if (!mib_FindEntry(tableIndex, &mibOps, &pMibOps))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "cannot find onu power shedding mib entry");

        return;
    }

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,
        "power post-shedding timer %hu timeout", instanceID);

    // delete timer
    omci_timer_delete_by_id(classID, instanceID);

    if (OMCI_TIMER_PWR_POST_SHED_DATA_INST_ID == instanceID)
    {
        // stop power shedding
        if (pMibOps->SheddingStatus & OPS_SHEDDING_STATUS_DATA_CLASS)
        {
            tableIndex = MIB_TABLE_ETHUNI_INDEX;
            entrySize = MIB_GetTableEntrySize(tableIndex);
            portPwrDown.state = FALSE;

            ret = MIB_GetFirst(tableIndex, &mibPptpEthUNI, entrySize);
            while (GOS_OK == ret)
            {
                ret = pptp_eth_uni_me_id_to_switch_port(mibPptpEthUNI.EntityID, &portId);
                if (GOS_OK == ret)
                {
                    portPwrDown.port = portId;

                    ret = omci_wrapper_setPhyPwrDown(&portPwrDown);

                    if (GOS_OK != ret)
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                            "stop shedding power for data port %hu fail", portId);
                    }
                    else
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_WARN,
                            "stop shedding power for data port %hu", portId);
                    }
                }

                ret = MIB_GetNext(tableIndex, &mibPptpEthUNI, entrySize);
            }

            // update shedding status
            pMibOps->SheddingStatus &= ~OPS_SHEDDING_STATUS_DATA_CLASS;
        }

        // clear post-shed timer
        pMibOps->DataResetIntvlRemained = 0;
        // reset pre-shed timer
        pMibOps->DataShedIntvlRemained = pMibOps->DataShedIntvl;
    }

    if (OMCI_TIMER_PWR_POST_SHED_VOICE_INST_ID == instanceID)
    {
        // stop power shedding
        if (pMibOps->SheddingStatus & OPS_SHEDDING_STATUS_VOICE_CLASS)
        {
            // TBD

            // update shedding status
            pMibOps->SheddingStatus &= ~OPS_SHEDDING_STATUS_VOICE_CLASS;
        }

        // clear post-shed timer
        pMibOps->VoiceResetIntvlRemained = 0;
        // reset pre-shed timer
        pMibOps->VoiceShedIntvlRemained = pMibOps->VoiceShedIntvl;
    }
}

GOS_ERROR_CODE omci_pwr_shedding_timer_handler(omci_alm_status_t    almStatus,
                                                UINT32              preShedTimerId,
                                                UINT32              postShedTimerId,
                                                UINT16              *pShedIntvlRemained,
                                                UINT16              *pResetIntvlRemained)
{
    GOS_ERROR_CODE      ret;
    omci_timer_entry_t  *pPreShedEntry;
    omci_timer_entry_t  *pPostShedEntry;
    time_t              remainedSecs;
    long                remainedNanoSecs;

    // find associated timer
    pPreShedEntry = omci_timer_search(
        OMCI_TIMER_RESERVED_CLASS_ID, preShedTimerId);
    pPostShedEntry = omci_timer_search(
        OMCI_TIMER_RESERVED_CLASS_ID, postShedTimerId);

    if (OMCI_ALM_STS_DECLARE == almStatus)
    {
        if (pPostShedEntry)
        {
            // stop and delete post-shed timer
            ret = omci_timer_stop_and_delete_by_id(
                    OMCI_TIMER_RESERVED_CLASS_ID,
                    postShedTimerId, &remainedSecs, &remainedNanoSecs);

            if (GOS_OK != ret)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                    "stop/delete power post-shed timer %u fail", postShedTimerId);

                return ret;
            }

            // record remained secs before expiration
            *pResetIntvlRemained = remainedSecs + 1;

            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "stop/delete power post-shed timer %u", postShedTimerId);
        }

        if (!pPreShedEntry)
        {
            // create pre-shed timer
            ret = omci_timer_create(
                    OMCI_TIMER_RESERVED_CLASS_ID,
                    preShedTimerId, *pShedIntvlRemained, 0, FALSE,
                    0, omci_pwr_shedding_pre_shed_handler,OMCI_TIMER_SIG_SHEDDING);

            if (GOS_OK != ret)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                    "create power pre-shed timer %u fail", preShedTimerId);

                return ret;
            }

            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "create power pre-shed timer %u", preShedTimerId);
        }
    }
    else
    {
        if (pPreShedEntry)
        {
            // stop and delete pre-shed timer
            ret = omci_timer_stop_and_delete_by_id(
                    OMCI_TIMER_RESERVED_CLASS_ID,
                    preShedTimerId, &remainedSecs, &remainedNanoSecs);

            if (GOS_OK != ret)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                    "stop/delete power pre-shed timer %u fail", preShedTimerId);

                return ret;
            }

            // record remained secs before expiration
            *pShedIntvlRemained = remainedSecs + 1;

            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "stop/delete power pre-shed timer %u", preShedTimerId);
        }

        if (!pPostShedEntry)
        {
            // create post-shed timer
            ret = omci_timer_create(
                    OMCI_TIMER_RESERVED_CLASS_ID,
                    postShedTimerId, *pResetIntvlRemained, 1, FALSE,
                    0, omci_pwr_shedding_post_shed_handler,OMCI_TIMER_SIG_SHEDDING);

            if (GOS_OK != ret)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                    "create power post-shed timer %u fail", postShedTimerId);

                return ret;
            }

            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "create power post-shed timer %u", postShedTimerId);
        }
    }

    return GOS_OK;
}

GOS_ERROR_CODE omci_pwr_shedding_processor(omci_alm_status_t almStatus)
{
    GOS_ERROR_CODE                  ret;
    MIB_TABLE_INDEX                 tableIndex;
    MIB_TABLE_ONU_PWR_SHEDDING_T    mibOps;
    MIB_TABLE_ONU_PWR_SHEDDING_T    *pMibOps;

    // get power shedding data
    tableIndex = MIB_TABLE_ONU_PWR_SHEDDING_INDEX;
    mibOps.EntityId = 0;

    if (!mib_FindEntry(tableIndex, &mibOps, &pMibOps))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "cannot find onu power shedding mib entry");

        return GOS_ERR_NOT_FOUND;
    }

    if (pMibOps->DataShedIntvl != OPS_SHEDDING_INTVL_DISABLE_SHEDDING)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "processing power shedding timer for data class");

        ret = omci_pwr_shedding_timer_handler(almStatus,
                OMCI_TIMER_PWR_PRE_SHED_DATA_INST_ID,
                OMCI_TIMER_PWR_POST_SHED_DATA_INST_ID,
                &pMibOps->DataShedIntvlRemained,
                &pMibOps->DataResetIntvlRemained);

        if (GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "processing power shedding timer for data class fail");

            return ret;
        }
    }

    if (pMibOps->VoiceShedIntvl != OPS_SHEDDING_INTVL_DISABLE_SHEDDING)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "processing power shedding timer for voice class");

        ret = omci_pwr_shedding_timer_handler(almStatus,
                OMCI_TIMER_PWR_PRE_SHED_VOICE_INST_ID,
                OMCI_TIMER_PWR_POST_SHED_VOICE_INST_ID,
                &pMibOps->VoiceShedIntvlRemained,
                &pMibOps->VoiceResetIntvlRemained);

        if (GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "processing power shedding timer for voice class fail");

            return ret;
        }
    }

    return GOS_OK;
}

BOOL omci_is_one_pptp_eth_uni_number_in_bridge(MIB_TREE_T *pTree, UINT16 *pId)
{
    MIB_NODE_T              *pNode = NULL;
    UINT32                  number = 0;
    BOOL                    ret = FALSE;
    MIB_TABLE_ETHUNI_T      *pMibPptpEthUNI = NULL;
    MIB_TREE_NODE_ENTRY_T   *pNodeEntry = NULL;
    MIB_ENTRY_T             *pEntry = NULL;

    if (!pTree || !pId)
        return FALSE;

    if ((pNode = MIB_AvlTreeSearch(pTree->root, AVL_KEY_PPTPUNI)))
    {
        LIST_FOREACH(pNodeEntry, &pNode->data.treeNodeEntryHead, treeNodeEntry)
        {
            number++;
            if (!LIST_NEXT(pNodeEntry, treeNodeEntry))
                continue;
        }
    }

    if (1 == number && pNode)
    {
        LIST_FOREACH(pNodeEntry, &pNode->data.treeNodeEntryHead, treeNodeEntry)
        {
            if (!(pEntry = pNodeEntry->mibEntry))
                return ret;

            if (!(pMibPptpEthUNI = (MIB_TABLE_ETHUNI_T *)pEntry->pData))
                return ret;

            *pId = pMibPptpEthUNI->EntityID;
        }
        ret = TRUE;
    }
    return ret;
}

GOS_ERROR_CODE omci_get_pptp_eth_uni_port_mask_in_bridge(UINT16 mbspId, UINT32 *pPortMask)
{
    MIB_TABLE_ETHUNI_T      *pMibPptpEthUNI;
    MIB_TREE_T              *pTree;
    MIB_NODE_T              *pNode;
    MIB_TREE_NODE_ENTRY_T   *pNodeEntry;
    MIB_ENTRY_T             *pEntry;
    UINT16                  portId;

    if (!pPortMask)
        return GOS_ERR_PARAM;

    *pPortMask = 0;

    pTree = MIB_AvlTreeSearchByKey(NULL, mbspId, AVL_KEY_MACBRISERVPROF);
    if (pTree)
    {
        pNode = MIB_AvlTreeSearch(pTree->root, AVL_KEY_PPTPUNI);
        if (pNode)
        {
            LIST_FOREACH(pNodeEntry, &pNode->data.treeNodeEntryHead, treeNodeEntry)
            {
                pEntry = pNodeEntry->mibEntry;
                pMibPptpEthUNI = (MIB_TABLE_ETHUNI_T *)pEntry->pData;

                if (GOS_OK == pptp_eth_uni_me_id_to_switch_port(pMibPptpEthUNI->EntityID, &portId))
                {
                    *pPortMask |= (1 << portId);
                }
            }
        }
    }

    return GOS_OK;
}

GOS_ERROR_CODE omci_update_dot1_rate_limiter_port_mask(UINT16   mbspId,
                                                        UINT32  newPortMask,
                                                        UINT32  oldPortMask)
{
    GOS_ERROR_CODE                  ret;
    MIB_TABLE_DOT1_RATE_LIMITER_T   mibDrl;
    MIB_TABLE_TRAFFICDESCRIPTOR_T   mibTd;
    omci_dot1_rate_t                dot1Rate;
    MIB_TABLE_INDEX                 tableIndex;
    UINT32                          entrySize;
    UINT8                           bFound = FALSE;

    tableIndex = MIB_TABLE_DOT1_RATE_LIMITER_INDEX;
    entrySize = MIB_GetTableEntrySize(tableIndex);

    ret = MIB_GetFirst(tableIndex, &mibDrl, entrySize);
    while (GOS_OK == ret)
    {
        if ((mibDrl.ParentMePtr == mbspId) && (mibDrl.TpType == DOT1_RATE_LIMITER_TP_TYPE_MAC_BRIDGE))
        {
            bFound = TRUE;
            break;
        }

        ret = MIB_GetNext(tableIndex, &mibDrl, entrySize);
    }

    // just return ok if no limiter is associated to this bridge
    if (!bFound)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "no associated dot1 rate limited is found");

        return GOS_OK;
    }

    // deal with unicast flood
    ret = omci_is_traffic_descriptor_existed(mibDrl.UsUcFloodRatePtr, &mibTd);
    if (GOS_OK == ret)
    {
        dot1Rate.type = OMCI_DOT1_RATE_UNICAST_FLOOD;

        if (oldPortMask != 0)
        {
            dot1Rate.portMask = oldPortMask;

            ret = omci_wrapper_delDot1RateLimiter(&dot1Rate);
            if (GOS_OK != ret)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "del dot1 rate limiter for uc flood fail");

                return GOS_FAIL;
            }
        }

        dot1Rate.portMask = newPortMask;
        dot1Rate.cir = mibTd.CIR;
        dot1Rate.cbs = mibTd.CBS;

        ret = omci_wrapper_setDot1RateLimiter(&dot1Rate);
        if (GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "set dot1 rate limiter for uc flood fail");

            return GOS_FAIL;
        }
    }

    // deal with broadcast
    ret = omci_is_traffic_descriptor_existed(mibDrl.UsBcRatePtr, &mibTd);
    if (GOS_OK == ret)
    {
        dot1Rate.type = OMCI_DOT1_RATE_BROADCAST;

        if (oldPortMask != 0)
        {
            dot1Rate.portMask = oldPortMask;

            ret = omci_wrapper_delDot1RateLimiter(&dot1Rate);
            if (GOS_OK != ret)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "del dot1 rate limiter for bc fail");

                return GOS_FAIL;
            }
        }

        dot1Rate.portMask = newPortMask;
        dot1Rate.cir = mibTd.CIR;
        dot1Rate.cbs = mibTd.CBS;

        ret = omci_wrapper_setDot1RateLimiter(&dot1Rate);
        if (GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "set dot1 rate limiter for bc fail");

            return GOS_FAIL;
        }
    }

    // deal with multicast payload
    ret = omci_is_traffic_descriptor_existed(mibDrl.UsMcPayloadRatePtr, &mibTd);
    if (GOS_OK == ret)
    {
        dot1Rate.type = OMCI_DOT1_RATE_MULTICAST_PAYLOAD;

        if (oldPortMask != 0)
        {
            dot1Rate.portMask = oldPortMask;

            ret = omci_wrapper_delDot1RateLimiter(&dot1Rate);
            if (GOS_OK != ret)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "del dot1 rate limiter for mc payload fail");

                return GOS_FAIL;
            }
        }

        dot1Rate.portMask = newPortMask;
        dot1Rate.cir = mibTd.CIR;
        dot1Rate.cbs = mibTd.CBS;

        ret = omci_wrapper_setDot1RateLimiter(&dot1Rate);
        if (GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "set dot1 rate limiter for mc payload fail");

            return GOS_FAIL;
        }
    }

    return GOS_OK;
}

BOOL TR069ManageServerRelatedServiceCheck(MIB_TABLE_IP_HOST_CFG_DATA_T **ppMibIpHostCfgData)
{
    MIB_TABLE_VEIP_T veip, *pMibVeip = NULL;
    MIB_TABLE_TR069MANAGESERVER_T tr069, *pMibTR069 = NULL;
    MIB_TABLE_TCP_UDP_CFG_DATA_T tcpUdpCfgData, *pMibTcpUdpCfgData = NULL;
    MIB_TABLE_IP_HOST_CFG_DATA_T ipHostCfgData;
    UINT32                       veipSlotId;

    veipSlotId = TXC_CARDHLD_VEIP_SLOT_TYPE_ID;
    feature_api(FEATURE_API_ME_00000100, &veipSlotId);


    veip.EntityId = (UINT16)((veipSlotId << 8) | 1);
    OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN,
        (!mib_FindEntry(MIB_TABLE_VEIP_INDEX, &veip, &pMibVeip)), FALSE);

    tr069.EntityID = veip.EntityId;
    OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN,
        (!mib_FindEntry(MIB_TABLE_TR069MANAGESERVER_INDEX, &tr069, &pMibTR069)), FALSE);

    tcpUdpCfgData.EntityId = pMibVeip->TcpUdpPtr;
    OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN,
        (!mib_FindEntry(MIB_TABLE_TCP_UDP_CFG_DATA_INDEX, &tcpUdpCfgData, &pMibTcpUdpCfgData)), FALSE);

    ipHostCfgData.EntityID = pMibTcpUdpCfgData->IpHostPointer;
    OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN,
        (!mib_FindEntry(MIB_TABLE_IP_HOST_CFG_DATA_INDEX, &ipHostCfgData, ppMibIpHostCfgData)), FALSE);

    return TRUE;

}

static GOS_ERROR_CODE TR069ManageServerRelatedAcsCheck(
    UINT16 acsAddrId,
    MIB_TABLE_LARGE_STRING_T **ppMibLargeString,
    MIB_TABLE_AUTH_SEC_METHOD_T **ppMibAuthenSecMethod)
{
    MIB_TABLE_NETWORK_ADDR_T networkAddr, *pMibNetworkAddr = NULL;
    MIB_TABLE_LARGE_STRING_T largeString;
    MIB_TABLE_AUTH_SEC_METHOD_T authSecMethod;

    networkAddr.EntityId = acsAddrId;
    OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN,
        (!mib_FindEntry(MIB_TABLE_NETWORK_ADDR_INDEX, &networkAddr, &pMibNetworkAddr)), OP_RESET_ACS);

    largeString.EntityId = pMibNetworkAddr->AddrPtr;
    OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN,
        (!mib_FindEntry(MIB_TABLE_LARGE_STRING_INDEX, &largeString, ppMibLargeString)), OP_RESET_ACS);

    authSecMethod.EntityId = pMibNetworkAddr->SecurityPtr;
    OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN,
        (!mib_FindEntry(MIB_TABLE_AUTH_SEC_METHOD_INDEX, &authSecMethod, ppMibAuthenSecMethod)), OP_SET_ACS);

    return OP_SET_ACS;
}

GOS_ERROR_CODE omci_check_iphost_relation_by_service(
    UINT32 service_type, MIB_TABLE_IP_HOST_CFG_DATA_T **ppMibIpHostCfgData, BOOL *pUnlockB)
{

    MIB_TABLE_VEIP_T                mibVeip;
    MIB_TABLE_TR069MANAGESERVER_T   mibTr69;
    MIB_TABLE_TCP_UDP_CFG_DATA_T    mibTcpUdp;
    MIB_TABLE_IP_HOST_CFG_DATA_T    mibIpHostCfgData;
    MIB_TABLE_VOIPCONFIGDATA_T      mibVoipCfgData;
    MIB_TABLE_SIPAGENTCONFIGDATA_T  mibSipAgentCfgData;
    UINT32                          veipSlotId, SipAgentCfgDataNum = 0, count = 0;


    if (service_type != IF_SERVICE_TR69 && service_type != IF_SERVICE_SIP)
        return GOS_FAIL;

    if (IF_SERVICE_TR69 == service_type)
    {
        veipSlotId = TXC_CARDHLD_VEIP_SLOT_TYPE_ID;
        feature_api(FEATURE_API_ME_00000100, &veipSlotId);
        mibVeip.EntityId = (UINT16)((veipSlotId << 8) | 1);
        mibTr69.EntityID = mibVeip.EntityId;

        if (GOS_OK != MIB_Get(MIB_TABLE_VEIP_INDEX, &mibVeip, sizeof(MIB_TABLE_VEIP_T)))
            return GOS_FAIL;

        if (GOS_OK != MIB_Get(MIB_TABLE_TR069MANAGESERVER_INDEX, &mibTr69, sizeof(MIB_TABLE_TR069MANAGESERVER_T)))
            return GOS_FAIL;

        mibTcpUdp.EntityId = mibVeip.TcpUdpPtr;

        if (GOS_OK != MIB_Get(MIB_TABLE_TCP_UDP_CFG_DATA_INDEX, &mibTcpUdp, sizeof(MIB_TABLE_TCP_UDP_CFG_DATA_T)))
            return GOS_FAIL;

        mibIpHostCfgData.EntityID = mibTcpUdp.IpHostPointer;
        if (!mib_FindEntry(MIB_TABLE_IP_HOST_CFG_DATA_INDEX, &mibIpHostCfgData, ppMibIpHostCfgData))
            return GOS_FAIL;

        *pUnlockB = (0 == mibTr69.AdminState ? TRUE : FALSE);
        feature_api(FEATURE_API_ME_00000010, &mibVeip, pUnlockB);
        return GOS_OK;
    }
    else
    {
        mibVoipCfgData.EntityId = 0;
        if (GOS_OK != MIB_Get(MIB_TABLE_VOIPCONFIGDATA_INDEX, &mibVoipCfgData, sizeof(MIB_TABLE_VOIPCONFIGDATA_T)))
            return GOS_FAIL;

        switch (mibVoipCfgData.SignallingProtocolUsed)
        {
            case VCD_SIG_PROTOCOL_USED_SIP:

                if (0 == (SipAgentCfgDataNum = MIB_GetTableCurEntryCount(MIB_TABLE_SIPAGENTCONFIGDATA_INDEX)))
                    return GOS_FAIL;

                if (GOS_OK != (MIB_GetFirst(MIB_TABLE_SIPAGENTCONFIGDATA_INDEX, &mibSipAgentCfgData, sizeof(MIB_TABLE_SIPAGENTCONFIGDATA_T))))
                    return GOS_FAIL;

                while (count < SipAgentCfgDataNum)
                {
                    count++;

                    mibTcpUdp.EntityId = mibSipAgentCfgData.TCPUDPPointer;
                    if (GOS_OK != MIB_Get(MIB_TABLE_TCP_UDP_CFG_DATA_INDEX, &mibTcpUdp, sizeof(MIB_TABLE_TCP_UDP_CFG_DATA_T)))
                        goto next_sip_agent;

                    mibIpHostCfgData.EntityID = mibTcpUdp.IpHostPointer;
                    if (mib_FindEntry(MIB_TABLE_IP_HOST_CFG_DATA_INDEX, &mibIpHostCfgData, ppMibIpHostCfgData))
                        return GOS_OK;

                next_sip_agent:
                    if (GOS_OK != (MIB_GetNext(MIB_TABLE_SIPAGENTCONFIGDATA_INDEX, &mibSipAgentCfgData, sizeof(MIB_TABLE_SIPAGENTCONFIGDATA_T))))
                        return GOS_FAIL;
                }
                break;
            case VCD_SIG_PROTOCOL_USED_H248:
                //TBD
                break;
            default:
                break;
        }
        return GOS_FAIL;
    }
}


void omci_setup_mgmt_interface(UINT32 op_id, UINT32 wan_type, UINT32 service_type, MIB_TABLE_IP_HOST_CFG_DATA_T *pIpHost, UINT32 *pUnlockB)
{
    mgmt_cfg_msg_t      mgmtInfo;
    // set if tci by tr142 api
    memset(&mgmtInfo, 0, sizeof(mgmt_cfg_msg_t));
    mgmtInfo.cfg.if_entry.if_id = pIpHost->EntityID;
    mgmtInfo.cfg.if_entry.if_tci = 0xFFFF;
    mgmtInfo.op_id = op_id;

    mgmtInfo.cfg.if_entry.if_is_ipv6_B = FALSE;
    mgmtInfo.cfg.if_entry.if_is_DHCP_B = (pIpHost->IpOptions &
                                        IP_HOST_CFG_DATA_IP_OPTIONS_ENABLE_DHCP) ? TRUE : FALSE;

    if(!mgmtInfo.cfg.if_entry.if_is_DHCP_B)
    {
        memcpy(&mgmtInfo.cfg.if_entry.ip_addr.ipv4_addr,
            &(pIpHost->IpAddress), sizeof(omci_ipv4_addr_t));
        memcpy(&mgmtInfo.cfg.if_entry.mask_addr,
            &(pIpHost->Mask), sizeof(omci_ipv4_addr_t));
        memcpy(&mgmtInfo.cfg.if_entry.gateway_addr,
            &(pIpHost->Gateway), sizeof(omci_ipv4_addr_t));
        memcpy(&mgmtInfo.cfg.if_entry.primary_dns_addr,
            &(pIpHost->PrimaryDns), sizeof(omci_ipv4_addr_t));
        memcpy(&mgmtInfo.cfg.if_entry.second_dns_addr,
            &(pIpHost->SecondaryDns), sizeof(omci_ipv4_addr_t));
    }

    mgmtInfo.cfg.if_entry.if_is_ip_stack_B = (pIpHost->IpOptions &
                                            IP_HOST_CFG_DATA_IP_OPTIONS_ENABLE_IP_STACK) ? TRUE : FALSE;
    if (IF_CHANNEL_MODE_PPPOE == wan_type)
        mgmtInfo.cfg.if_entry.if_is_ip_stack_B = TRUE;

    mgmtInfo.cfg.if_entry.if_service_type = service_type;
    if (IF_SERVICE_TR69 == service_type && pUnlockB)
    {
        // unlock: ONU can try to connect with ACS server; // lock: ONU cannot try to connect with ACS server
        mgmtInfo.cfg.if_entry.if_service_type = (*pUnlockB ? IF_SERVICE_TR69 : IF_SERVICE_DATA);
    }

    mgmtInfo.cfg.if_entry.wan_type = wan_type;
    feature_api(FEATURE_API_L3SVC_MGMT_CFG_SET, &mgmtInfo, sizeof(mgmt_cfg_msg_t));
    omci_update_if_vlan(&(pIpHost->EntityID));

    return;
}

void omci_setup_mgmt_acs_info(MIB_TABLE_TR069MANAGESERVER_T *pMibTR069)
{
    MIB_TABLE_LARGE_STRING_T *pMibLargeString = NULL;
    MIB_TABLE_AUTH_SEC_METHOD_T *pMibAuthenSecMethod = NULL;
    MIB_TABLE_IP_HOST_CFG_DATA_T *pMibIpHostCfgData = NULL;
    mgmt_cfg_msg_t mgmtInfo;
    UINT32 relatedID = UINT_MAX;
    UINT32 loop, offset = 0;

    memset(&mgmtInfo, 0, sizeof(mgmt_cfg_msg_t));

    mgmtInfo.op_id = TR069ManageServerRelatedAcsCheck(
                        pMibTR069->AcsAddress, &pMibLargeString, &pMibAuthenSecMethod);

    if (pMibLargeString)
    {
        for (loop = 0; loop < pMibLargeString->NumOfParts; loop++)
        {
            offset = MIB_TABLE_LARGE_STRING_PART_LEN * loop;

            memcpy(mgmtInfo.cfg.acs.acs_url + offset,
                (UINT8 *)pMibLargeString->Part1 + offset + loop, MIB_TABLE_LARGE_STRING_PART_LEN);
        }
    }

    if (TR069ManageServerRelatedServiceCheck(&pMibIpHostCfgData))
        relatedID = pMibIpHostCfgData->EntityID;

    mgmtInfo.cfg.acs.related_if_id = relatedID;

    if (pMibAuthenSecMethod)
    {
        memcpy(mgmtInfo.cfg.acs.username, (UINT8 *)(pMibAuthenSecMethod->Username1),
            strlen(pMibAuthenSecMethod->Username1) + 1);
        memcpy(mgmtInfo.cfg.acs.password, (UINT8 *)(pMibAuthenSecMethod->Password),
            strlen(pMibAuthenSecMethod->Password) + 1);
    }

    feature_api(FEATURE_API_L3SVC_MGMT_CFG_SET, &mgmtInfo, sizeof(mgmt_cfg_msg_t));

    return;
}

GOS_ERROR_CODE proprietary_list_proc(UINT8 cb_type)
{
    GOS_ERROR_CODE      ret = GOS_OK;
    MIB_TABLE_T         *pTable = NULL;
    proprietary_mib_t   *pMib = NULL;
    UINT8               cbBitMask = (1 << cb_type);

    LIST_FOREACH(pMib, &proprietaryMibHead, entries)
    {
        if (pMib->cbBitMask & cbBitMask)
        {
            pTable = mib_GetTablePtr(pMib->tableId);
            if (pTable && pTable->meOper && pTable->meOper->meOperCb[cb_type])
            {
                if (GOS_OK != (ret = pTable->meOper->meOperCb[cb_type]()))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                            "meOperCb[%u] handler returns fail: %s",
                            cb_type,
                            MIB_GetTableName(pMib->tableId));
                }
            }
        }
    }

    return GOS_OK;
}

static void omci_msg_exp_timer_handler(UINT16   classID,
                                        UINT16  instanceID,
                                        UINT32  privData)
{
    GOS_ERROR_CODE      ret = GOS_OK;
    omci_event_msg_t    omciEventMsg;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Message expiration timer expired");

    proprietary_list_proc(PROPRIETARY_MIB_CB_UPDATE);

    omciEventMsg.subType = OMCI_EVENT_TYPE_CFG_CHANGE;
    omciEventMsg.status = DISABLED;

    ret = omci_wrapper_sendOmciEvent(&omciEventMsg);
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Send cfg change event fail");
    }

    // delete timer
    omci_timer_delete_by_id(classID, instanceID);
}

GOS_ERROR_CODE omci_msg_exp_timer_processor()
{
    GOS_ERROR_CODE      ret = GOS_OK;
    omci_timer_entry_t  *pEntry;
    omci_event_msg_t    omciEventMsg;

    pEntry = omci_timer_search(OMCI_TIMER_RESERVED_CLASS_ID,
                                OMCI_TIMER_RESERVED_MSG_EXP_INST_ID);

    if (pEntry)
    {
        // restart timer
        ret = omci_timer_restart(OMCI_TIMER_RESERVED_CLASS_ID,
                                OMCI_TIMER_RESERVED_MSG_EXP_INST_ID,
                                OMCI_TIMER_MSG_EXPIRED_SECS,
                                0, FALSE);
        if (GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Restart timer for msg expire fail");

            return GOS_FAIL;
        }
    }
    else
    {
        // create timer for msg expire monitoring
        ret = omci_timer_create(OMCI_TIMER_RESERVED_CLASS_ID,
                                OMCI_TIMER_RESERVED_MSG_EXP_INST_ID,
                                OMCI_TIMER_MSG_EXPIRED_SECS,
                                0, FALSE, 0, omci_msg_exp_timer_handler,OMCI_TIMER_SIG_MSG_EXT);
        if (GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Create timer for msg expire fail");

            return GOS_FAIL;
        }

        omciEventMsg.subType = OMCI_EVENT_TYPE_CFG_CHANGE;
        omciEventMsg.status = ENABLED;

        ret = omci_wrapper_sendOmciEvent(&omciEventMsg);
        if (GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Send cfg change event fail");

            omci_timer_delete_by_id(OMCI_TIMER_RESERVED_CLASS_ID,
                                    OMCI_TIMER_RESERVED_MSG_EXP_INST_ID);

            return GOS_FAIL;
        }
    }

    return GOS_OK;
}

GOS_ERROR_CODE omci_reset_onu_to_factory_default()
{
    OMCI_PRINT("[omci] receive reset default, proceeding...");

    OMCI_TaskDelay(100);

    system("echo 1 > /proc/load_default");

    return GOS_OK;
}

GOS_ERROR_CODE OMCI_TasksDump_Cmd(void)
{
    int oldstdout = -1, nul = -1;
    const int stdout_fd = fileno(stdout);
    nul = omci_open_cli_fd();

    if (-1 != nul)
    {
        oldstdout = dup(stdout_fd);
        dup2(nul, stdout_fd);
        close(nul);
    }

    OMCI_DumpTask();

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

GOS_ERROR_CODE omci_mib_oper_dump_default_handler(void *pData, MIB_TABLE_INFO_T *pTblInfo)
{
    MIB_ATTR_INFO_T     *pAttrInfo;
    UINT8               *pDataPtr;
    UINT8               isAllPrintable;
    UINT32              i, j ,attrLen;
    MIB_TABLE_INDEX         tableIndex;

    if (!pData || !pTblInfo || !pTblInfo->pAttributes)
        return GOS_OK;

    tableIndex = MIB_GetTableIndexByClassId(pTblInfo->ClassId);

    for (i = 0; i < pTblInfo->attrNum; i++)
    {
        pAttrInfo = &pTblInfo->pAttributes[i];
        pDataPtr = mib_GetAttrPtr(
            MIB_GetTableIndexByClassId(pTblInfo->ClassId), pData, i+1);
        attrLen  = MIB_GetAttrLen(tableIndex, i+1);

        switch (pAttrInfo->DataType)
        {
            case MIB_ATTR_TYPE_UINT8:
            {
                UINT8   data;
                memcpy(&data, (UINT8 *)pDataPtr, sizeof(UINT8));
                if (MIB_ATTR_OUT_HEX == pAttrInfo->OutStyle)
                {
                    OMCI_PRINT("%s: 0x%02x", pAttrInfo->Name, data);
                }
                else
                {
                    OMCI_PRINT("%s: %hhu", pAttrInfo->Name, data);
                }
                break;
            }
            case MIB_ATTR_TYPE_UINT16:
            {
                UINT16  data;
                memcpy(&data, (UINT16 *)pDataPtr, sizeof(UINT16));
                if (MIB_ATTR_OUT_HEX == pAttrInfo->OutStyle)
                {
                    OMCI_PRINT("%s: 0x%04x", pAttrInfo->Name, data);
                }
                else
                {
                    OMCI_PRINT("%s: %hu", pAttrInfo->Name, data);
                }
                break;
            }
            case MIB_ATTR_TYPE_UINT32:
            {
                UINT32  data;
                memcpy(&data, (UINT32 *)pDataPtr, sizeof(UINT32));
                if (MIB_ATTR_OUT_HEX == pAttrInfo->OutStyle)
                {
                    OMCI_PRINT("%s: 0x%08x", pAttrInfo->Name, data);
                }
                else
                {
                    OMCI_PRINT("%s: %u", pAttrInfo->Name, data);
                }
                break;
            }
            case MIB_ATTR_TYPE_UINT64:
            {
                UINT64  data;
                data = GOS_BuffToUINT64((CHAR *)pDataPtr, attrLen);
                if (MIB_ATTR_OUT_HEX == pAttrInfo->OutStyle)
                {
                    OMCI_PRINT("%s: 0x%08x%08x",
                        pAttrInfo->Name, data.high, data.low);
                }
                else
                {
                    OMCI_PRINT("%s: %llu", pAttrInfo->Name,
                        ((unsigned long long)data.high << 32) | data.low);
                }
                break;
            }
            case MIB_ATTR_TYPE_STR:
            {
                isAllPrintable = TRUE;
                for (j = 0; j < pAttrInfo->Len; j++)
                {
                    if (0 == isprint(pDataPtr[j]))
                        isAllPrintable = FALSE;
                }
                if (isAllPrintable)
                {
                    OMCI_PRINT("%s: %s",
                        pAttrInfo->Name, pDataPtr);
                }
                else
                {
                    printf("%s: 0x", pAttrInfo->Name);
                    for (j = 0; j < pAttrInfo->Len; j++)
                        printf("%02x", pDataPtr[j]);
                    printf("\n");
                }
                pDataPtr++;
                break;
            }
            default:
                break;
        }
    }

    return GOS_OK;
}

int omci_open_cli_fd(void)
{
    char                cmd[256], buffer[256];
    char                *p      = NULL, *string = NULL;
    FILE                *pFd    = NULL;
    unsigned int    count = 0;
    long int           pid = -1;
    int                 nul = -1;

    memset(cmd, 0, 256);
    memset(buffer, 0, 256);
    snprintf(cmd, sizeof(cmd), "ps | grep omcicli");

    if (NULL != (pFd = popen(cmd, "r")))
    {
        if (fgets(buffer, sizeof(buffer), pFd))
        {
            string = buffer;
            while ((p = strsep(&string, " ")) != NULL)
            {
                if (strlen(p) <= 0)
                    continue;
                if (0 == count)
                {
                    pid = strtol(p, NULL, 10);
                }
                count++;
            }
        }
        pclose(pFd);
    }
    if (-1 == pid)
        return  nul;

    memset(cmd, 0, 256);
    snprintf(cmd, sizeof(cmd), "/proc/%ld/fd/1", pid);
    nul = open(cmd, O_RDWR);

    if (nul < 0)
        OMCI_PRINT("open failed stdout of pid [%ld]\n\n", pid);

    return nul;
}

