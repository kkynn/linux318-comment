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
 * Purpose : Definition of OMCI message APIs
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) OMCI message APIs
 */

#include "app_basic.h"
#include "feature_mgmt.h"


#define OMCI_HIS_RSP_MSG_MAX_NUM (16)


UINT32 gOmciMsgNum         = 0;
UINT32 gOmciCrcErr         = 0;
BOOL   gOmciCrcCheckEnable = FALSE;
BOOL   gOmciOmitErrEnable  = FALSE;

omci_msg_norm_baseline_t gOmciHistoryRspMsg[OMCI_MSG_BASELINE_PRI_NUM][OMCI_HIS_RSP_MSG_MAX_NUM];
UINT32           gOmciLastRspMsgIndex[OMCI_MSG_BASELINE_PRI_NUM];


extern FILE *pLogFile;
extern FILE *pLogFileParsed;


/*
*  Define local function for message handler
*/

static UINT32* omci_CoeffCRC32(void)
{
    static BOOL   inited = FALSE;
    static UINT32 crctable[256];

    if (inited)
    {
        return crctable;
    }

    inited = TRUE;

    {
        register UINT32 i;
        register UINT32 j;
        UINT32          accum  = 0;
        UINT32          poly   = 0;
        INT32           pn32[] =
                        {
                        //  x32 + x26 + x23 + x22 + x16 + x12 + x11 + x10 + x8 + x7 + x5 + x4 + x2 + x + 1
                            0, 1, 2, 4, 5, 7, 8, 10, 11, 12, 16, 22, 23, 26
                        };


        for (i = 0; i < (sizeof(pn32)/sizeof(pn32[0])); i++)
        {
           poly = poly | (1UL<<pn32[i]);
        }

        for (i = 0; i < 256; i++)
        {
           accum = i << 24;
           for (j = 0; j < 8; j++)
           {
               if (accum & (1L << 31))
               {
                   accum = (accum << 1) ^ poly;
               }
               else
               {
                   accum = accum << 1;
               }
           }
           crctable[i] = accum;
        }
    }

    return crctable;
}


UINT32 omci_CalcCRC32(UINT32 accum, const UINT8* pBuf, UINT32 size)
{
    UINT32  i;
    UINT32* table = omci_CoeffCRC32();

    GOS_ASSERT(pBuf);

    accum = ~accum;

    for (i = 0; i < size; i++)
    {
        accum =  table[((accum >> 24) ^ pBuf[i]) & 0xff] ^ (accum << 8);
    }

    return ~accum;
}


static GOS_ERROR_CODE omci_CheckCRC(omci_msg_baseline_fmt_t* pOmciMsg)
{
    UINT32 crc = 0;

    crc = omci_CalcCRC32(crc, (const UINT8*)pOmciMsg, sizeof(omci_msg_baseline_fmt_t) - sizeof(pOmciMsg->trailer.crc));

    if (crc != GOS_Ntohl(pOmciMsg->trailer.crc))
    {
        gOmciCrcErr++;
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"CRC is incorrect (expected: 0x%x, received: 0x%x), total %d CRC incorrect",
                  crc, pOmciMsg->trailer.crc, gOmciCrcErr);
        return GOS_ERR_CRC;
    }

    return GOS_OK;
}



static GOS_ERROR_CODE omci_CheckMsgHdr(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    if (OMCI_MSG_BASELINE_DEVICE_ID != pNorOmciMsg->devId)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Device ID is incorrect (expected: 0x%x, received: 0x%x)",
                  OMCI_MSG_BASELINE_DEVICE_ID, pNorOmciMsg->devId);
        return GOS_FAIL;
    }


    // all OMCI messages should set AR bit on except for download section
    if (!pNorOmciMsg->ar && OMCI_MSG_TYPE_DOWNLOAD_SECTION != pNorOmciMsg->type)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"AR bit incorrect");
        return GOS_FAIL;
    }

    if (pNorOmciMsg->ak)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"AK bit incorrect");
        return GOS_FAIL;
    }

    return GOS_OK;
}


static void omci_ParseMsgAttrsName(MIB_TABLE_INDEX tableIndex, omci_msg_attr_mask_t omciAttrsSet, int *buffLen, CHAR *buff)
{
    MIB_ATTR_INDEX attrIndex;
    MIB_ATTRS_SET mibAttrsSet;
    int i;

    omci_AttrsOmciSetToMibSet(&omciAttrsSet, &mibAttrsSet);

    for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0; i < MIB_GetTableAttrNum(tableIndex);
            i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        if (MIB_IsInAttrSet(&mibAttrsSet, attrIndex))
        {
            *buffLen += sprintf((CHAR *)buff + *buffLen,"%s,",
                                MIB_GetAttrName(tableIndex, attrIndex));
        }
    }
}

static void omci_ParseMsgAttrs(UINT8 * pOmciData, MIB_TABLE_INDEX tableIndex, omci_msg_attr_mask_t omciAttrsSet, int *buffLen, CHAR *buff)
{
    MIB_ATTR_INDEX attrIndex;
    MIB_ATTR_TYPE attrType;
    MIB_ATTRS_SET mibAttrsSet;
    UINT32 attrLen;
    int i, j;
    //CHAR *pTemp;

    omci_AttrsOmciSetToMibSet(&omciAttrsSet, &mibAttrsSet);

    for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0; i < MIB_GetTableAttrNum(tableIndex);
            i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        if (MIB_IsInAttrSet(&mibAttrsSet, attrIndex))
        {
            attrLen  = MIB_GetAttrLen(tableIndex, attrIndex);
            attrType = MIB_GetAttrDataType(tableIndex, attrIndex);

            switch (attrType)
            {
                case MIB_ATTR_TYPE_UINT8:
                {
                    *buffLen += sprintf((CHAR *)buff + *buffLen,"%s=0x%02x,",
                                MIB_GetAttrName(tableIndex, attrIndex), (UINT8)*pOmciData);
                    break;
                }

                case MIB_ATTR_TYPE_UINT16:
                {
                    *buffLen += sprintf((CHAR *)buff + *buffLen,"%s=0x%04x,",
                                MIB_GetAttrName(tableIndex, attrIndex), GOS_Ntohs(GOS_GetUINT16((UINT16*)pOmciData)));
                    break;
                }

                case MIB_ATTR_TYPE_UINT32:
                {
                    *buffLen += sprintf((CHAR *)buff + *buffLen,"%s=0x%08x,",
                                MIB_GetAttrName(tableIndex, attrIndex), GOS_Ntohl(GOS_GetUINT32((UINT32*)pOmciData)));

                    break;
                }

                case MIB_ATTR_TYPE_UINT64:
                {
                    UINT64 u64Value = GOS_BuffToUINT64((CHAR*)pOmciData, attrLen);

                    *buffLen += sprintf((CHAR *)buff + *buffLen,"%s=0x%08x%08x,",
                                MIB_GetAttrName(tableIndex, attrIndex), u64Value.high, u64Value.low);

                    break;
                }

                case MIB_ATTR_TYPE_STR:
                case MIB_ATTR_TYPE_TABLE:
                {
                    *buffLen += sprintf((CHAR *)buff + *buffLen,"%s=0x",
                                MIB_GetAttrName(tableIndex, attrIndex));
                    for(j = 0; j < attrLen; j++)
                    {
                        *buffLen += sprintf((CHAR *)buff + *buffLen,"%02x", (UINT8)(*(pOmciData+j)));
                    }
                    *buffLen += sprintf((CHAR *)buff + *buffLen,",");
                    break;
                }

                default:
                    break;
            }
            pOmciData  += attrLen;
        }
    }
}

static void omci_logMsg(UINT8* msg)
{
    int i;
    struct timespec ts;

    if(pLogFile && msg &&
        ((1 << (msg[2]& OMCI_MSG_TYPE_MT_MASK)) & gInfo.gLoggingActMask))
    {
        /*Print time stamp, precision is millisec*/
        if(gInfo.logFileMode & OMCI_LOGFILE_MODE_WITH_TIMESTAMP)
        {
            clock_gettime(CLOCK_MONOTONIC, &ts);
            fprintf(pLogFile, "[%ld.%03ld] ",
                ts.tv_sec, ts.tv_nsec/1000000);
        }

        for(i = 0; i < 48; i++)
        {
            fprintf(pLogFile, "%02x ", msg[i]);
        }
        fprintf(pLogFile, "\n");
        fflush(pLogFile);
    }
}

static void omci_LogParsedMsg(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    MIB_TABLE_INDEX tableIndex;
    omci_msg_attr_mask_t omciAttrsSet;
    //MIB_ATTRS_SET mibAttrsSet;
    struct timespec ts;

    int buffLen = 0;
    CHAR buff[512];

    memset(buff, 0 , sizeof(buff));

    if(pLogFileParsed == NULL)
    {
        return;
    }

    /*check message action is need to be logged*/
    if (!((1 << pNorOmciMsg->type) & gInfo.gLoggingActMask))
    {
        return;
    }

    tableIndex = MIB_GetTableIndexByClassId((omci_me_class_t)pNorOmciMsg->meId.meClass);

    /*Print time stamp, precision is millisec*/
    if(gInfo.logFileMode & OMCI_LOGFILE_MODE_WITH_TIMESTAMP)
    {
        clock_gettime(CLOCK_MONOTONIC, &ts);
        buffLen = sprintf(buff, "[%ld.%03ld] ",
            ts.tv_sec, ts.tv_nsec/1000000);
    }

    /*Unknow ME*/
    if (tableIndex == MIB_TABLE_UNKNOWN_INDEX)
    {
        buffLen += sprintf(buff + buffLen, "0x%04x UnknowME %d\n",
        pNorOmciMsg->tcId, pNorOmciMsg->meId.meClass);
        /*Print buff to file*/
        fprintf(pLogFileParsed, "%s", buff);
        fflush(pLogFileParsed);
        return;
    }

    buffLen += sprintf(buff + buffLen, "0x%04x %s ",
        pNorOmciMsg->tcId, MIB_GetTableName(tableIndex));


    if(!pNorOmciMsg->ak){
        switch(pNorOmciMsg->type)
        {
            case OMCI_MSG_TYPE_CREATE:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "Create(0x%04x,", pNorOmciMsg->meId.meInstance);

                omciAttrsSet = omci_GetOltAccAttrSet(tableIndex, OMCI_ME_ATTR_ACCESS_SBC);
                omci_ParseMsgAttrs(&pNorOmciMsg->content[0], tableIndex, omciAttrsSet, &buffLen, buff);
                buffLen += sprintf((CHAR *)buff + (buffLen - 1), ")\n");
                break;
            }
            case OMCI_MSG_TYPE_DELETE:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "Delete(0x%04x)\n", pNorOmciMsg->meId.meInstance);
                break;
            }
            case OMCI_MSG_TYPE_SET:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "Set(0x%04x,", pNorOmciMsg->meId.meInstance);

                omciAttrsSet = (omci_msg_attr_mask_t)GOS_Ntohs(GOS_GetUINT16((UINT16*)&pNorOmciMsg->content[0]));
                omci_ParseMsgAttrs(&pNorOmciMsg->content[2], tableIndex, omciAttrsSet, &buffLen, buff);
                buffLen += sprintf((CHAR *)buff + (buffLen - 1), ")\n");
                break;
            }
            case OMCI_MSG_TYPE_GET:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "Get(0x%04x,", pNorOmciMsg->meId.meInstance);

                omciAttrsSet = (omci_msg_attr_mask_t)GOS_Ntohs(GOS_GetUINT16((UINT16*)&pNorOmciMsg->content[0]));
                omci_ParseMsgAttrsName(tableIndex, omciAttrsSet, &buffLen, buff);
                buffLen += sprintf((CHAR *)buff + (buffLen - 1), ")\n");
                break;
            }
            case OMCI_MSG_TYPE_GET_ALL_ALARMS:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "GetAllAlarm(0x%04x)\n", pNorOmciMsg->meId.meInstance);
                break;
            }
            case OMCI_MSG_TYPE_GET_ALL_ALARMS_NEXT:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "GetAllAlarmNext(0x%04x)\n", pNorOmciMsg->meId.meInstance);
                break;
            }
            case OMCI_MSG_TYPE_MIB_UPLOAD:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "MibUpload(0x%04x)\n", pNorOmciMsg->meId.meInstance);
                break;
            }
            case OMCI_MSG_TYPE_MIB_UPLOAD_NEXT:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "MibUploadNext(SeqNum %d)\n",
                    GOS_Ntohs(GOS_GetUINT16((UINT16*)&pNorOmciMsg->content[0])));
                break;
            }
            case OMCI_MSG_TYPE_MIB_RESET:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "MibReset(0x%04x)\n", pNorOmciMsg->meId.meInstance);
                break;
            }
            case OMCI_MSG_TYPE_ALARM:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "Alarm(0x%04x)\n", pNorOmciMsg->meId.meInstance);
                /*Context TBD*/
                break;
            }
            case OMCI_MSG_TYPE_ATTR_VALUE_CHANGE:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "AVC(0x%04x)\n", pNorOmciMsg->meId.meInstance);
                /*Context TBD*/
                break;
            }
            case OMCI_MSG_TYPE_TEST:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "Test(0x%04x)\n", pNorOmciMsg->meId.meInstance);
                /*Context TBD*/
                break;
            }
            case OMCI_MSG_TYPE_START_SW_DOWNLOAD:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "StartSwDownload(0x%04x)\n", pNorOmciMsg->meId.meInstance);
                /*Context TBD*/
                break;
            }
            case OMCI_MSG_TYPE_DOWNLOAD_SECTION:
            {
                if (pNorOmciMsg->ar)
                    buffLen += sprintf((CHAR *)buff + buffLen, "DownloadSection(0x%04x)\n", pNorOmciMsg->meId.meInstance);
                /*Context TBD*/
                break;
            }
            case OMCI_MSG_TYPE_END_SW_DOWNLOAD:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "EndSwDownload(0x%04x)\n", pNorOmciMsg->meId.meInstance);
                /*Context TBD*/
                break;
            }
            case OMCI_MSG_TYPE_ACTIVATE_SW:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "ActivateSw(0x%04x)\n", pNorOmciMsg->meId.meInstance);
                /*Context TBD*/
                break;
            }
            case OMCI_MSG_TYPE_COMMIT_SW:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "CommitSw(0x%04x)\n", pNorOmciMsg->meId.meInstance);
                break;
            }
            case OMCI_MSG_TYPE_SYNCHRONIZE_TIME:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "SyncTime(0x%04x)\n", pNorOmciMsg->meId.meInstance);
                break;
            }
            case OMCI_MSG_TYPE_REBOOT:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "Reboot(0x%04x)\n", pNorOmciMsg->meId.meInstance);
                /*Context TBD*/
                break;
            }
            case OMCI_MSG_TYPE_GET_NEXT:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "GetNext(0x%04x)\n", pNorOmciMsg->meId.meInstance);
                /*Context TBD*/
                break;
            }
            case OMCI_MSG_TYPE_TEST_RESULT:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "TestResult(0x%04x)\n", pNorOmciMsg->meId.meInstance);
                /*Context TBD*/
                break;
            }
            case OMCI_MSG_TYPE_GET_CURRENT_DATA:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "GetCurrentData(0x%04x,", pNorOmciMsg->meId.meInstance);
                omciAttrsSet = (omci_msg_attr_mask_t)GOS_Ntohs(GOS_GetUINT16((UINT16*)&pNorOmciMsg->content[0]));
                omci_ParseMsgAttrsName(tableIndex, omciAttrsSet, &buffLen, buff);
                buffLen += sprintf((CHAR *)buff + (buffLen - 1), ")\n");
                break;
            }
            default:
                break;

        }
    }
    else
    {
        switch(pNorOmciMsg->type)
        {
            case OMCI_MSG_TYPE_CREATE:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "CreateRsp(0x%04x,Result=0x%x)\n",
                    pNorOmciMsg->meId.meInstance, (UINT8)pNorOmciMsg->content[0]);
                break;
            }
            case OMCI_MSG_TYPE_DELETE:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "DeleteRsp(0x%04x,Result=0x%x)\n",
                    pNorOmciMsg->meId.meInstance, (UINT8)pNorOmciMsg->content[0]);
                break;
            }
            case OMCI_MSG_TYPE_SET:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "SetRsp(0x%04x,Result=0x%x)\n",
                    pNorOmciMsg->meId.meInstance, (UINT8)pNorOmciMsg->content[0]);
                break;
            }
            case OMCI_MSG_TYPE_GET:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "GetRsp(0x%04x,Result=0x%x,",
                    pNorOmciMsg->meId.meInstance, (UINT8)pNorOmciMsg->content[0]);

                omciAttrsSet = (omci_msg_attr_mask_t)GOS_Ntohs(GOS_GetUINT16((UINT16*)&pNorOmciMsg->content[1]));
                omci_ParseMsgAttrs(&pNorOmciMsg->content[3], tableIndex, omciAttrsSet, &buffLen, buff);
                buffLen += sprintf((CHAR *)buff + (buffLen - 1), ")\n");
                break;
            }
            case OMCI_MSG_TYPE_GET_ALL_ALARMS:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "GetAllAlarmRsp(0x%04x)\n",
                    pNorOmciMsg->meId.meInstance);
                /*Context TBD*/
                break;
            }
            case OMCI_MSG_TYPE_GET_ALL_ALARMS_NEXT:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "GetAllAlarmNextRsp(0x%04x)\n",
                    pNorOmciMsg->meId.meInstance);
                /*Context TBD*/
                break;
            }
            case OMCI_MSG_TYPE_MIB_UPLOAD:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "MibUploadRsp(0x%04x, SubSeqNum %d)\n",
                    pNorOmciMsg->meId.meInstance, GOS_Ntohs(GOS_GetUINT16((UINT16*)&pNorOmciMsg->content[0])));
                break;
            }
            case OMCI_MSG_TYPE_MIB_UPLOAD_NEXT:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "MibUploadNextRsp(");

                tableIndex = MIB_GetTableIndexByClassId((omci_me_class_t)GOS_Ntohs(GOS_GetUINT16((UINT16*)&pNorOmciMsg->content[0])));
                if (tableIndex == MIB_TABLE_UNKNOWN_INDEX)
                {
                    buffLen += sprintf(buff + buffLen, "UnknowME %d)\n",
                        (omci_me_class_t)GOS_Ntohs(GOS_GetUINT16((UINT16*)&pNorOmciMsg->content[0])));
                } else {
                    buffLen += sprintf((CHAR *)buff + buffLen, "%s 0x%04x, ", MIB_GetTableName(tableIndex), GOS_Ntohs(GOS_GetUINT16((UINT16*)&pNorOmciMsg->content[2])));
                    omciAttrsSet = (omci_msg_attr_mask_t)GOS_Ntohs(GOS_GetUINT16((UINT16*)&pNorOmciMsg->content[4]));
                    omci_ParseMsgAttrs(&pNorOmciMsg->content[6], tableIndex, omciAttrsSet, &buffLen, buff);
                    buffLen += sprintf((CHAR *)buff + (buffLen - 1), ")\n");
                }
                break;
            }
            case OMCI_MSG_TYPE_MIB_RESET:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "MibResetRsp(0x%04x,Result=0x%x)\n",
                    pNorOmciMsg->meId.meInstance, (UINT8)pNorOmciMsg->content[0]);
                break;
            }
            case OMCI_MSG_TYPE_TEST:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "TestRsp(0x%04x,Result=0x%x)\n",
                    pNorOmciMsg->meId.meInstance, (UINT8)pNorOmciMsg->content[0]);
                break;
            }
            case OMCI_MSG_TYPE_START_SW_DOWNLOAD:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "StartSwDownloadRsp(0x%04x)\n",
                    pNorOmciMsg->meId.meInstance);
                /*Context TBD*/
                break;
            }
            case OMCI_MSG_TYPE_DOWNLOAD_SECTION:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "DownloadSectionRsp(0x%04x)\n",
                    pNorOmciMsg->meId.meInstance);
                /*Context TBD*/
                break;
            }
            case OMCI_MSG_TYPE_END_SW_DOWNLOAD:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "EndSwDownloadRsp(0x%04x,Result=0x%x)\n",
                    pNorOmciMsg->meId.meInstance, (UINT8)pNorOmciMsg->content[0]);
                /*Context TBD*/
                break;
            }
            case OMCI_MSG_TYPE_ACTIVATE_SW:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "ActivateSwRsp(0x%04x,Result=0x%x)\n",
                    pNorOmciMsg->meId.meInstance, (UINT8)pNorOmciMsg->content[0]);
                break;
            }
            case OMCI_MSG_TYPE_COMMIT_SW:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "CommitSwRsp(0x%04x,Result=0x%x)\n",
                    pNorOmciMsg->meId.meInstance, (UINT8)pNorOmciMsg->content[0]);
                break;
            }
            case OMCI_MSG_TYPE_SYNCHRONIZE_TIME:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "SyncTimeRsp(0x%04x,Result=0x%x)\n",
                    pNorOmciMsg->meId.meInstance, (UINT8)pNorOmciMsg->content[0]);
                break;
            }
            case OMCI_MSG_TYPE_REBOOT:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "RebootRsp(0x%04x,Result=0x%x)\n",
                    pNorOmciMsg->meId.meInstance, (UINT8)pNorOmciMsg->content[0]);
                break;
            }
            case OMCI_MSG_TYPE_GET_NEXT:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "GetNextRsp(0x%04x)\n",
                    pNorOmciMsg->meId.meInstance);
                /*Context TBD*/
                break;
            }
            case OMCI_MSG_TYPE_GET_CURRENT_DATA:
            {
                buffLen += sprintf((CHAR *)buff + buffLen, "GetCurrentDataRsp(0x%04x,Result=0x%x,",
                    pNorOmciMsg->meId.meInstance, (UINT8)pNorOmciMsg->content[0]);
                omciAttrsSet = (omci_msg_attr_mask_t)GOS_Ntohs(GOS_GetUINT16((UINT16*)&pNorOmciMsg->content[1]));
                omci_ParseMsgAttrs(&pNorOmciMsg->content[3], tableIndex, omciAttrsSet, &buffLen, buff);
                buffLen += sprintf((CHAR *)buff + (buffLen - 1), ")\n");
                break;
            }
            default:
                break;
        }
    }

    /*Print buff to file*/
    fprintf(pLogFileParsed, "%s", buff);
    fflush(pLogFileParsed);
}



static GOS_ERROR_CODE omci_SendOmciMsg(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    omci_msg_baseline_fmt_t rawOmciMsg;

    if (PON_ONU_STATE_OPERATION != gInfo.onuState)
        return GOS_OK;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Send out OMCI msg");

    gOmciMsgNum++;

    pNorOmciMsg->trailer.cpcsLen  = 0x0028;
    pNorOmciMsg->trailer.cpcsUuCpi = 0;

    rawOmciMsg.tcId = GOS_Htons(pNorOmciMsg->tcId | (pNorOmciMsg->priority << OMCI_MSG_BASELINE_PRI_BIT));
    rawOmciMsg.type = (pNorOmciMsg->db << OMCI_MSG_TYPE_DB_BIT) |
                      (pNorOmciMsg->ar << OMCI_MSG_TYPE_AR_BIT) |
                      (pNorOmciMsg->ak << OMCI_MSG_TYPE_AK_BIT) |
                      (pNorOmciMsg->type & OMCI_MSG_TYPE_MT_MASK);


    rawOmciMsg.devId = (UINT8)pNorOmciMsg->devId;

    rawOmciMsg.meId.meClass    = GOS_Htons(pNorOmciMsg->meId.meClass);
    rawOmciMsg.meId.meInstance = GOS_Htons(pNorOmciMsg->meId.meInstance);

    memcpy(rawOmciMsg.content, pNorOmciMsg->content, OMCI_MSG_BASELINE_CONTENTS_LEN);

    rawOmciMsg.trailer.cpcsLen  = GOS_Htons(pNorOmciMsg->trailer.cpcsLen);
    rawOmciMsg.trailer.cpcsUuCpi = GOS_Htons(pNorOmciMsg->trailer.cpcsUuCpi);
    rawOmciMsg.trailer.crc  = GOS_Htonl(omci_CalcCRC32(0, (const UINT8*)&rawOmciMsg, sizeof(omci_msg_baseline_fmt_t) - sizeof(rawOmciMsg.trailer.crc)));

    pNorOmciMsg->trailer.crc = GOS_Ntohl(rawOmciMsg.trailer.crc);


    if (gInfo.logFileMode & (OMCI_LOGFILE_MODE_CONSOLE + OMCI_LOGFILE_MODE_RAW))
        omci_logMsg((UINT8 *)(&rawOmciMsg));

    if(gInfo.logFileMode & (OMCI_LOGFILE_MODE_CONSOLE + OMCI_LOGFILE_MODE_PARSED))
    {
        omci_LogParsedMsg(pNorOmciMsg);
    }

    return omci_wrapper_sendOmciMsg(&rawOmciMsg, sizeof(omci_msg_baseline_fmt_t));
}




static omci_msg_norm_baseline_t* omci_GetRspMsgBuff(UINT16 pri)
{
    GOS_ASSERT(pri < OMCI_MSG_BASELINE_PRI_NUM);
    GOS_ASSERT(gOmciLastRspMsgIndex[pri] < OMCI_HIS_RSP_MSG_MAX_NUM);

    if (gOmciLastRspMsgIndex[pri] == OMCI_HIS_RSP_MSG_MAX_NUM - 1)
    {
        gOmciLastRspMsgIndex[pri] = 0;
    }
    else
    {
        gOmciLastRspMsgIndex[pri]++;
    }

    memset(&gOmciHistoryRspMsg[pri][gOmciLastRspMsgIndex[pri]], 0x00, sizeof(omci_msg_norm_baseline_t));

    return &gOmciHistoryRspMsg[pri][gOmciLastRspMsgIndex[pri]];
}



static GOS_ERROR_CODE omci_CheckDuplicateMsgRsp(omci_msg_norm_baseline_t* pMsg)
{
    UINT32 i;
    GOS_ERROR_CODE ret = GOS_OK;

    GOS_ASSERT(pMsg->priority < OMCI_MSG_BASELINE_PRI_NUM);

    if (0 == pMsg->tcId)
        return ret;

    for (i = 0; i < OMCI_HIS_RSP_MSG_MAX_NUM; i++)
    {
        if ((gOmciHistoryRspMsg[pMsg->priority][i].tcId == pMsg->tcId)
            && (gOmciHistoryRspMsg[pMsg->priority][i].meId.meClass == pMsg->meId.meClass)
            && (gOmciHistoryRspMsg[pMsg->priority][i].meId.meInstance == pMsg->meId.meInstance)
            && (gOmciHistoryRspMsg[pMsg->priority][i].type == pMsg->type))
        {
            omci_SendOmciMsg(&(gOmciHistoryRspMsg[pMsg->priority][i]));
            return GOS_FAIL;
        }
    }

    return ret;
}

static GOS_ERROR_CODE omci_DispatchMsg(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    switch(pNorOmciMsg->type)
    {
        case OMCI_MSG_TYPE_CREATE:
        {
            omci_msg_exp_timer_processor();
            return OMCI_OnCreateMsg(pNorOmciMsg);
        }
        case OMCI_MSG_TYPE_DELETE:
        {
            omci_msg_exp_timer_processor();
            return OMCI_OnDeleteMsg(pNorOmciMsg);
        }
        case OMCI_MSG_TYPE_SET:
        {
            omci_msg_exp_timer_processor();
            return OMCI_OnSetMsg(pNorOmciMsg);
        }
        case OMCI_MSG_TYPE_GET:
        {
            return OMCI_OnGetMsg(pNorOmciMsg);
        }
        case OMCI_MSG_TYPE_MIB_UPLOAD:
        {
            omci_msg_exp_timer_processor();
            return OMCI_OnMibUploadMsg(pNorOmciMsg);
        }
        case OMCI_MSG_TYPE_MIB_UPLOAD_NEXT:
        {
            omci_msg_exp_timer_processor();
            return OMCI_OnMibUploadNextMsg(pNorOmciMsg);
        }
        case OMCI_MSG_TYPE_MIB_RESET:
        {
            omci_msg_exp_timer_processor();
            return OMCI_OnMibReset(pNorOmciMsg);
        }
        case OMCI_MSG_TYPE_ACTIVATE_SW:
        {
            return OMCI_OnActivateSw(pNorOmciMsg);
        }
        case OMCI_MSG_TYPE_COMMIT_SW:
        {
            return OMCI_OnCommitSw(pNorOmciMsg);
        }
        case OMCI_MSG_TYPE_REBOOT:
        {
            return OMCI_OnReboot(pNorOmciMsg);
        }
        case OMCI_MSG_TYPE_GET_NEXT:
        {
            return OMCI_OnGetNextMsg(pNorOmciMsg);
        }
        case OMCI_MSG_TYPE_GET_ALL_ALARMS:
        {
            return OMCI_OnGetAllAlarms(pNorOmciMsg);
        }
        case OMCI_MSG_TYPE_GET_ALL_ALARMS_NEXT:
        {
            return OMCI_OnGetAllAlarmsNext(pNorOmciMsg);
        }
        case OMCI_MSG_TYPE_ALARM:
        {
            OMCI_LOG(OMCI_LOG_LEVEL_INFO,"Handling Action: Alarm\n");
            return GOS_OK;
        }
        case OMCI_MSG_TYPE_TEST:
        {
            return OMCI_OnTest(pNorOmciMsg);
        }
        case OMCI_MSG_TYPE_START_SW_DOWNLOAD:
        {
            return OMCI_OnStartSWDL(pNorOmciMsg);
        }
        case OMCI_MSG_TYPE_DOWNLOAD_SECTION:
        {
            return OMCI_OnDLSection(pNorOmciMsg);
        }
        case OMCI_MSG_TYPE_END_SW_DOWNLOAD:
        {
            return OMCI_OnEndSWDL(pNorOmciMsg);
        }
        case OMCI_MSG_TYPE_SYNCHRONIZE_TIME:
        {
            return OMCI_OnSync(pNorOmciMsg);
        }
        case OMCI_MSG_TYPE_TEST_RESULT:
        {
            OMCI_LOG(OMCI_LOG_LEVEL_INFO,"Handling Action: Test Result\n");
            return GOS_OK;
        }
        case OMCI_MSG_TYPE_GET_CURRENT_DATA:
        {
            return OMCI_OnGetMsg(pNorOmciMsg);
        }
        default:
         break;
    }

    return GOS_FAIL;
}

static GOS_ERROR_CODE omci_HandleRxOmciMsg(omci_msg_baseline_fmt_t* pOmciMsg)
{
    omci_msg_norm_baseline_t  norOmciMsg;
    BOOL meValid;
    BOOL meExist;

    GOS_ASSERT(pOmciMsg);

    if (gInfo.logFileMode & (OMCI_LOGFILE_MODE_CONSOLE + OMCI_LOGFILE_MODE_RAW))
        omci_logMsg((UINT8 *)pOmciMsg);

    gOmciMsgNum++;

    if (gOmciCrcCheckEnable)
    {
        if (GOS_OK != omci_CheckCRC(pOmciMsg))
        {
            return GOS_ERR_CRC;
        }
    }

    OMCI_NormalizeMsg(pOmciMsg, &norOmciMsg);

    if (GOS_OK != omci_CheckMsgHdr(&norOmciMsg))
    {
        return GOS_FAIL;
    }

    /*Log Msg after check AK, for not log replayed Response Mag*/
    if(gInfo.logFileMode & (OMCI_LOGFILE_MODE_CONSOLE + OMCI_LOGFILE_MODE_PARSED))
    {
        omci_LogParsedMsg(&norOmciMsg);
    }

    // It's a duplicate OMCI message, retransmit its response message
    if (GOS_OK != omci_CheckDuplicateMsgRsp(&norOmciMsg))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,"Receive duplicated cmd, tcId = %d", norOmciMsg.tcId);

        return GOS_FAIL;
    }

    // Check if the ME is valid and exist or not
    if (GOS_OK != OMCI_CheckIsMeValid(norOmciMsg.meId.meClass, &norOmciMsg.meId.meInstance, &meValid, &meExist))
    {
        return GOS_FAIL;
    }

    if (meValid == FALSE)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,"Receive operation on unknown me, cmd = %d, me = %d, inst = %d",
              norOmciMsg.type, norOmciMsg.meId.meClass, norOmciMsg.meId.meInstance);
        OMCI_ResponseMsg(&norOmciMsg, OMCI_RSP_UNKNOWN_ME_CLASS, TRUE);
        //OMCI_ResponseMsg(&norOmciMsg, OMCI_RSP_PROCESSED_SUCCESSFULLY, TRUE);

        return GOS_FAIL;
    }

    if (norOmciMsg.type != OMCI_MSG_TYPE_CREATE && meExist == FALSE)
    {
        if (FAL_OK != feature_api(FEATURE_API_ME_00000800_IS_ENABLED,
                &norOmciMsg.meId.meClass, &norOmciMsg.meId.meInstance))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_WARN,"Receive operation on unknown inst, cmd = %d, me = %d, inst = %d",
                  norOmciMsg.type, norOmciMsg.meId.meClass, norOmciMsg.meId.meInstance);
            OMCI_ResponseMsg(&norOmciMsg, OMCI_RSP_UNKNOWN_ME_INSTANCE, TRUE);

            return GOS_FAIL;
        }
    }

    // Check if the ME action is supported or not
    if (OMCI_CheckIsActionSupported(norOmciMsg.meId.meClass, norOmciMsg.type) == FALSE)
    {
        //OMCI_ResponseMsg(&norOmciMsg, OMCI_RSP_CMD_NOT_SUPPORTED, TRUE);
        OMCI_ResponseMsg(&norOmciMsg, OMCI_RSP_PROCESSED_SUCCESSFULLY, TRUE);

        OMCI_LOG(OMCI_LOG_LEVEL_WARN,"Receive unsupported cmd, cmd = %d, me = %d",
                  norOmciMsg.type, norOmciMsg.meId.meClass);

        return GOS_ERR_NOTSUPPORT;
    }

    return omci_DispatchMsg(&norOmciMsg);
}


static GOS_ERROR_CODE omci_HandleTxOmciMsg(omci_msg_norm_baseline_t     *pNorOmciMsg)
{
    if (!pNorOmciMsg)
        return GOS_ERR_PARAM;
    return omci_SendOmciMsg(pNorOmciMsg);
}


static GOS_ERROR_CODE omci_HandleCmdMsg(PON_OMCI_CMD_T* pCmd, UINT32 srcMsgQkey)
{
    if (!pCmd)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s Error parameter",__FUNCTION__);
        return GOS_ERR_PARAM;
    }

    switch (pCmd->cmd)
    {
        case PON_OMCI_CMD_SN_SET:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Set serial number");
            OMCI_SnSet_Cmd(pCmd->sn);
            break;
        case PON_OMCI_CMD_SN_GET:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Get serial number");
            OMCI_SnGet_Cmd();
            break;
        case PON_OMCI_CMD_LOG_SET:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Set Driver log level usr:%u drv:%d", pCmd->usrLogLvl, pCmd->drvLogLvl);
            OMCI_LogSet_Cmd((int)pCmd->usrLogLvl, (int)pCmd->drvLogLvl);
            break;
        case PON_OMCI_CMD_LOG_GET:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Get log level");
            OMCI_LogGet_Cmd();
            break;
        case PON_OMCI_CMD_LOGFILE_SET:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Set log to file, mode %d, actmask %x", pCmd->state, pCmd->level);
            omci_wrapper_setLogToFile(pCmd->state, pCmd->level, pCmd->filename);
            break;
        case PON_OMCI_CMD_LOGFILE_GET:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Get log to file, mode and actmask");
            omci_wrapper_getLogToFileCfg();
            break;
        case PON_OMCI_CMD_TABLE_GET:
        {
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Get all tables");
            int oldstdout = -1, nul = -1;
            const int stdout_fd = fileno(stdout);
            nul = omci_open_cli_fd();

            if (-1 != nul)
            {
                oldstdout = dup(stdout_fd);
                dup2(nul, stdout_fd);
                close(nul);
            }
            MIB_ShowAll();
            if (-1 != nul)
            {
                fflush(stdout);
                if (-1 != oldstdout)
                {
                    dup2(oldstdout, stdout_fd);
                    close(oldstdout);
                }
            }
            break;
        }
        case PON_OMCI_CMD_DEVMODE_GET:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Get device mode");
            OMCI_DevModeGet_Cmd();
            break;
        case PON_OMCI_CMD_DEVMODE_SET:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Set device mode to %s", pCmd->filename);
            OMCI_DevModeSet_Cmd(pCmd->filename);
            break;
        case PON_OMCI_CMD_DUAL_MGMT_MODE_SET:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Set dual mgmt mode");
            omci_cmd_set_dmmode_handler(pCmd->state);
            break;
        case PON_OMCI_CMD_DUAL_MGMT_MODE_GET:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Get dual mgmt mode");
            omci_cmd_get_dmmode_handler();
            break;
        case PON_OMCI_CMD_LOID_SET:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Set LOID and password");
            OMCI_LoidSet_Cmd((unsigned char *)pCmd->filename, (unsigned char *)pCmd->value);
            break;
        case PON_OMCI_CMD_LOID_GET:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Get LOID and password");
            OMCI_LoidGet_Cmd();
            break;
        case PON_OMCI_CMD_LOIDAUTH_GET_RSP:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Get LOID auth status");
            OMCI_LoidAuthStatusGet_Cmd(srcMsgQkey);
            break;
        case PON_OMCI_CMD_LOIDAUTH_NUM_GET_RSP:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Get LOID auth Number");
            OMCI_LoidAuthNumGet_Cmd(srcMsgQkey);
            break;
        case PON_OMCI_CMD_LOIDAUTH_NUM_RESET:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Reset LOID auth Number");
            OMCI_LoidAuthNumReset_Cmd(srcMsgQkey);
            break;
        case PON_OMCI_CMD_OLT_LOCATION_GET_RSP:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Get OLT Location config data");
            OMCI_OltLocationGet_Cmd(srcMsgQkey);
            break;
        case PON_OMCI_CMD_IPHOST_DHCP_SET:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Set DHCP info of Iphost");
            printf("Set DHCP info of Iphost\n");
            OMCI_IphostDhcpSet_Cmd((if_info_t*)pCmd->value);
            break;
        case PON_OMCI_CMD_FAKE_OK_SET:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Set fake OK status");
            gOmciOmitErrEnable = pCmd->state;
            OMCI_Protocol_RspCtrl(&(pCmd->state));
            break;
        case PON_OMCI_CMD_PM_SET:
            {
                static BOOL     bRunningState = TRUE;
                BOOL            bPausePlayState = TRUE;

                OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Set PM running state");

                if (bRunningState != pCmd->state)
                {
                    omci_pm_pause_play_request_updater(TRUE, &bPausePlayState);
                    bRunningState = pCmd->state;
                }
            }
            break;
        case PON_OMCI_CMD_TM_SET:
            {
                BOOL            bPausePlayState = TRUE;

                OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Set TM running state");

                if (gInfo.tmTimerRunningB != pCmd->state)
                    omci_tm_pause_play_request_updater(TRUE, &bPausePlayState);
            }
            break;
        case PON_OMCI_CMD_DRV_VERSION_GET:
            OMCI_DrvVersionGet_Cmd();
            break;
        case PON_OMCI_CMD_IOT_VLAN_CFG_SET:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Set IOT vlan cfg");
            OMCI_IotVlanCfgSet_Cmd(pCmd->type, pCmd->mode, pCmd->pri, pCmd->vid);
            break;
        case PON_OMCI_CMD_CFLAG_SET:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Set cflag");
            OMCI_cFlagSet_Cmd(pCmd->value, pCmd->level);
            break;
        case PON_OMCI_CMD_CFLAG_GET:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Get cflag");
            OMCI_cFlagGet_Cmd();
            break;
        case PON_OMCI_CMD_DURATION_GET_RSP:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Get auth uptime");
            OMCI_AuthUptimeGet_Cmd(srcMsgQkey);
            break;

        case PON_OMCI_CMD_MIB_CREATE:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "MIB create");
            OMCI_MibCreate_Cmd(pCmd->tableId, pCmd->entityId, pCmd->value);
            break;
        case PON_OMCI_CMD_MIB_DELETE:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "MIB delete");
            OMCI_MibDelete_Cmd(pCmd->tableId, pCmd->entityId);
            break;
        case PON_OMCI_CMD_MIB_SET:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "MIB set");
            OMCI_MibSet_Cmd(pCmd->tableId, pCmd->entityId, pCmd->filename, (unsigned char *)pCmd->value);
            break;
        case PON_OMCI_CMD_MIB_GET:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "MIB get");
            OMCI_MibDump_Cmd(pCmd->tableId, pCmd->filename, pCmd->state);
            break;
        case PON_OMCI_CMD_MIB_GET_CURRENT:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "MIB get current");
            OMCI_MibDump_PM_Cmd(pCmd->filename, pCmd->state);
            break;
        case PON_OMCI_CMD_MIB_ALARM_GET:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "MIB alarm get");
            omci_cmd_dump_alarm_handler(pCmd->tableId, pCmd->entityId);
            break;
        case PON_OMCI_CMD_MIB_RESET:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "MIB reset");
            OMCI_MibReset_Cmd();
            break;
        case PON_OMCI_CMD_MIB_ATTR_GET_RSP:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "MIB get attr");
            OMCI_MibAttrGet_Cmd(srcMsgQkey, pCmd->tableId, pCmd->entityId, pCmd->state);
            break;

        case PON_OMCI_CMD_DUMP_AVL_TREE:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Dump AVL Tree");
            OMCI_TreeDump_Cmd(pCmd->state);
            break;
        case PON_OMCI_CMD_DUMP_QUEUE_MAP:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Dump TCONT Queue Mapping");
            OMCI_PriQtoTcontDump_Cmd();
            break;
        case PON_OMCI_CMD_DUMP_TREE_CONN:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Dump Tree Connections");
            OMCI_ConnDump_Cmd();
            break;
        case PON_OMCI_CMD_DUMP_SRV_FLOW:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Dump Service Flow");
            OMCI_ServiceFlowDump_Cmd();
            break;
        case PON_OMCI_CMD_DUMP_TASK:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Dump Tasks");
            OMCI_TasksDump_Cmd();
            break;

        case PON_OMCI_CMD_SIM_ALARM:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Simulate alarm");
            omci_cmd_send_alarm_handler(pCmd->tableId, pCmd->level, pCmd->state, pCmd->entityId);
            break;
        case PON_OMCI_CMD_SIM_AVC:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Simulate AVC");
            OMCI_SimAVC_Cmd(pCmd->tableId, pCmd->entityId, pCmd->type);
            break;

        case PON_OMCI_CMD_DETECT_IOT_VLAN:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Detect IOT vlan");
            OMCI_IotVlanDetect_Cmd();
            break;

        case PON_OMCI_CMD_GEN_DOT:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Generate dot file");
            OMCI_DotGen_Cmd();
            break;
        case PON_OMCI_CMD_SHOW_REG_MOD:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Show register feature module");
            OMCI_RegModShow_Cmd();
            break;
        case PON_OMCI_CMD_SHOW_REG_API:
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Show register feature api");
            OMCI_RegApiShow_Cmd();
            break;

        default:
            break;
    }

    return GOS_OK;
}




/*
*  Define Global APIs for Message handler
*/


GOS_ERROR_CODE OMCI_ResetHistoryRspMsg(void)
{
    memset(gOmciHistoryRspMsg, 0x00, OMCI_MSG_BASELINE_PRI_NUM * OMCI_HIS_RSP_MSG_MAX_NUM * sizeof(omci_msg_norm_baseline_t));

    return GOS_OK;
}


GOS_ERROR_CODE OMCI_ResetLastRspMsgIndex(void)
{
    memset(gOmciLastRspMsgIndex, 0x00, OMCI_MSG_BASELINE_PRI_NUM * sizeof(UINT32));

    return GOS_OK;
}



GOS_ERROR_CODE OMCI_HandleMsg(void* pData, OMCI_MSG_TYPE type, OMCI_MSG_PRI pri, UINT32 srcMsgQkey)
{
    GOS_ERROR_CODE ret = GOS_FAIL;
    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Rcv OMCI from MSG Queue!,type=%d\n",type);

    switch (type)
    {
        case OMCI_RX_OMCI_MSG:
        {
            ret = omci_HandleRxOmciMsg((omci_msg_baseline_fmt_t*)pData);
            break;
        }
        case OMCI_TX_OMCI_MSG:
        {
            ret = omci_HandleTxOmciMsg((omci_msg_norm_baseline_t *)pData);
            break;
        }
        case OMCI_ALARM_MSG:
        {
            ret = omci_ext_alarm_dispatcher((omci_alm_t *)pData);
            break;
        }
        case OMCI_CMD_MSG:
        {
           ret = omci_HandleCmdMsg((PON_OMCI_CMD_T*)pData, srcMsgQkey);
           break;
        }

        default:
        {
            break;
        }
    }

    return ret;
}



GOS_ERROR_CODE OMCI_SendIndicationToOlt(omci_msg_norm_baseline_t* pNorOmciMsg, UINT16 tr)
{
    pNorOmciMsg->priority = OMCI_MSG_BASELINE_PRI_LOW;
    pNorOmciMsg->tcId     = tr;
    pNorOmciMsg->db       = 0;
    pNorOmciMsg->ar       = 0;
    pNorOmciMsg->ak       = 0;
    pNorOmciMsg->devId    = OMCI_MSG_BASELINE_DEVICE_ID;

    if(FAL_OK == feature_api_is_registered(FEATURE_API_ME_00001000))
        return GOS_OK;

    return omci_SendOmciMsg(pNorOmciMsg);
}


GOS_ERROR_CODE OMCI_NormalizeMsg(omci_msg_baseline_fmt_t* pOmciMsg, omci_msg_norm_baseline_t* pNorOmciMsg)
{
    pNorOmciMsg->tcId     = GOS_Ntohs(pOmciMsg->tcId);
    pNorOmciMsg->priority = ((OMCI_MSG_BASELINE_PRI_MASK & pNorOmciMsg->tcId) ? OMCI_MSG_BASELINE_PRI_HIGH : OMCI_MSG_BASELINE_PRI_LOW);
    pNorOmciMsg->tcId     = OMCI_MSG_BASELINE_TCID_MASK & pNorOmciMsg->tcId;

    pNorOmciMsg->db    = (pOmciMsg->type & OMCI_MSG_TYPE_DB_MASK) ? 1 : 0;
    pNorOmciMsg->ar    = (pOmciMsg->type & OMCI_MSG_TYPE_AR_MASK) ? 1 : 0;
    pNorOmciMsg->ak    = (pOmciMsg->type & OMCI_MSG_TYPE_AK_MASK) ? 1 : 0;
    pNorOmciMsg->type  = (pOmciMsg->type & OMCI_MSG_TYPE_MT_MASK);
    pNorOmciMsg->devId = (UINT32)pOmciMsg->devId;

    pNorOmciMsg->meId.meClass    = GOS_Ntohs(pOmciMsg->meId.meClass);
    pNorOmciMsg->meId.meInstance = GOS_Ntohs(pOmciMsg->meId.meInstance);

    memcpy(&(pNorOmciMsg->content[0]), &(pOmciMsg->content[0]), OMCI_MSG_BASELINE_CONTENTS_LEN);

    pNorOmciMsg->trailer.crc  = GOS_Ntohl(pOmciMsg->trailer.crc);
    pNorOmciMsg->trailer.cpcsLen  = GOS_Ntohs(pOmciMsg->trailer.cpcsLen);
    pNorOmciMsg->trailer.cpcsUuCpi = GOS_Ntohs(pOmciMsg->trailer.cpcsUuCpi);

    return GOS_OK;
}




GOS_ERROR_CODE OMCI_ResponseMsg(omci_msg_norm_baseline_t* pNorOmciMsg, omci_msg_response_t result, BOOL clear)
{
    omci_msg_norm_baseline_t* pRspMsg = omci_GetRspMsgBuff(pNorOmciMsg->priority);

    GOS_ASSERT(pRspMsg != NULL);

    pRspMsg->priority         = pNorOmciMsg->priority;
    pRspMsg->tcId             = pNorOmciMsg->tcId;
    pRspMsg->db               = pNorOmciMsg->db;
    pRspMsg->ar               = 0;
    pRspMsg->ak               = 1;
    pRspMsg->type             = pNorOmciMsg->type;
    pRspMsg->devId            = pNorOmciMsg->devId;
    pRspMsg->meId.meClass    = pNorOmciMsg->meId.meClass;
    pRspMsg->meId.meInstance = pNorOmciMsg->meId.meInstance;

    if (clear)
    {
        memset(&(pNorOmciMsg->content[0]), 0x00, OMCI_MSG_BASELINE_CONTENTS_LEN);
    }

    if (gOmciOmitErrEnable)
    {
        if(OMCI_MSG_TYPE_START_SW_DOWNLOAD == pNorOmciMsg->type ||
                OMCI_MSG_TYPE_DOWNLOAD_SECTION == pNorOmciMsg->type ||
                OMCI_MSG_TYPE_END_SW_DOWNLOAD == pNorOmciMsg->type ||
                OMCI_MSG_TYPE_ACTIVATE_SW == pNorOmciMsg->type ||
                OMCI_MSG_TYPE_COMMIT_SW == pNorOmciMsg->type)
        {
            pNorOmciMsg->content[0] = (UINT8)result;
        }
        else if (OMCI_MSG_TYPE_GET_ALL_ALARMS != pNorOmciMsg->type &&
                OMCI_MSG_TYPE_GET_ALL_ALARMS_NEXT != pNorOmciMsg->type &&
                OMCI_MSG_TYPE_MIB_UPLOAD != pNorOmciMsg->type &&
                OMCI_MSG_TYPE_MIB_UPLOAD_NEXT != pNorOmciMsg->type &&
                OMCI_MSG_TYPE_ALARM != pNorOmciMsg->type &&
                OMCI_MSG_TYPE_ATTR_VALUE_CHANGE != pNorOmciMsg->type &&
                OMCI_MSG_TYPE_TEST_RESULT != pNorOmciMsg->type)
        {
            pNorOmciMsg->content[0] = (UINT8)OMCI_RSP_PROCESSED_SUCCESSFULLY;
        }
    }
    else
    {
        pNorOmciMsg->content[0] = (UINT8)result;
    }

    memcpy(&(pRspMsg->content[0]), &(pNorOmciMsg->content[0]), OMCI_MSG_BASELINE_CONTENTS_LEN);

    return omci_SendOmciMsg(pRspMsg);
}



