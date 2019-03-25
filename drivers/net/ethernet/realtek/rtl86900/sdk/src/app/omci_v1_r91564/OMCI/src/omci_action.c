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
 * Purpose : Definition of OMCI action APIs
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) OMCI action APIs
 */

#include "app_basic.h"
#include "omci_task.h"
#include "feature_mgmt.h"

extern int MIB_TABLE_TOTAL_NUMBER;
extern omciMibTables_t gTables;


#define OMCI_SW_DL_MAX_WINDOW_SIZE  (256)
#define OMCI_SW_DL_MAX_ACCEPTABLE_WINDOW_SIZE   (63)
#define OMCI_SW_DL_SECTION_BITMASK_IN_BYTE  (OMCI_SW_DL_MAX_WINDOW_SIZE / 8)
#define OMCI_SW_DL_MAX_BYTE_OF_SECTION_IN_BASELINE_MSG  (31)

#define OMCI_SW_DL_IMAGE_LOCATION   "/tmp/img.tar"

typedef struct
{
    UINT8   windowSize;
    UINT32  imageByteSize;

    /*
        by OMCI definition, the maximum window size is 256 (1 byte), which means
        we need 256 bit to record the download status of section inside the window
        since 256 bit equals to 32 bytes, we define a 32 bytes flag for this purpose
     */
    UINT8   dlSectionFlag[OMCI_SW_DL_SECTION_BITMASK_IN_BYTE];

    UINT8   *pDlWindowBuffer;

    BOOL    bCrcCheckDone;
    BOOL    bFlashingState;
} omci_sw_dl_ctrl_s;


/* multi get data */
omci_mulget_info_ts gOmciMulGetData[OMCI_MSG_BASELINE_PRI_NUM];
omci_fsm_id         gOmciMulGetFsm[OMCI_MSG_BASELINE_PRI_NUM];

/* mib upload data*/
omci_mib_upload_info_ts gOmciMibUploadData;
omci_fsm_id             gOmciMibUploadFsm;


// global software download control
omci_sw_dl_ctrl_s   gOmciSwDlCtrl;
pthread_t           gOmciSwDlUpgradeProcedureThreadID;
pthread_mutex_t     gOmciSwDlFlashingStateMutex = PTHREAD_MUTEX_INITIALIZER;

/* temp buffer */
CHAR temp[MIB_TABLE_ATTR_MAX_SIZE];
CHAR mulGetTempBuff[MIB_TABLE_ATTR_MAX_SIZE];

static void* omci_GetMibRowBuff(void);
static GOS_ERROR_CODE omci_InitFsm(void);
static omci_fsm_id omci_FsmCreate(UINT32 defaultState, UINT32 maxState, UINT32 maxEvent);
static GOS_ERROR_CODE omci_AddFsmEventHandler(omci_fsm_id fsmId, UINT32 state, UINT32 event, OMCI_FSMEVTHDL handler);
static GOS_ERROR_CODE omci_FsmRun(omci_fsm_id fsmId, UINT32 event, void* pEvtArg);
static GOS_ERROR_CODE omci_OmciDataToMibRow(MIB_TABLE_INDEX tableIndex, UINT8* pOmciData, void* pMibRow, omci_msg_attr_mask_t omciAttrSet);
static GOS_ERROR_CODE omci_SyncMibData(void);
static GOS_ERROR_CODE omci_CreateRelatedMe(MIB_TABLE_INDEX tableIndex, void* pMibRow);
static GOS_ERROR_CODE omci_SingleSet(omci_msg_norm_baseline_t* pNorOmciMsg);
static omci_msg_attr_mask_t omci_GetOptionAttrSet(MIB_TABLE_INDEX tableIndex, omci_me_attr_type_mask_t optType);
static GOS_ERROR_CODE omci_SingleGet(omci_msg_norm_baseline_t* pNorOmciMsg);
static MIB_ATTRS_SET omci_MibRowToOmciData(MIB_TABLE_INDEX tableIndex, void* pMibRow, UINT8* pOmciData, MIB_ATTRS_SET* pMibAttrSet);
static GOS_ERROR_CODE omci_MultiGet(omci_msg_norm_baseline_t* pNorOmciMsg);
static UINT32 omci_MulGetStartOnIdle(omci_msg_norm_baseline_t* pNorOmciMsg);
//static UINT32 omci_GetTableAttrLen(MIB_TABLE_INDEX tableIndex, MIB_ATTR_INDEX attrIndex);
static UINT32 omci_MulGetGetOnIdle(omci_msg_norm_baseline_t* pNorOmciMsg);
static UINT32 omci_MulGetStartOnRun(omci_msg_norm_baseline_t* pNorOmciMsg);
static UINT32 omci_MulGetGetOnRun(omci_msg_norm_baseline_t* pNorOmciMsg);
static UINT32 omci_MibUploadStartOnIdle(omci_msg_norm_baseline_t* pNorOmciMsg);
static UINT16 omci_GetMibUploadMaxSeqNum(void);
static UINT16 omci_GetSeqNumPerRow(MIB_TABLE_INDEX tableIndex);
static GOS_ERROR_CODE omci_GetMibUploadData ( UINT16 seqNum, CHAR* pDataBuf, UINT32 size, MIB_TABLE_INDEX* pTableIndex, omci_me_instance_t* pEntityId, MIB_ATTRS_SET*      pMibAttrSet );
static UINT32 omci_MibUploadGetOnIdle(omci_msg_norm_baseline_t* pNorOmciMsg);
static UINT32 omci_MibUploadStartOnRun(omci_msg_norm_baseline_t* pNorOmciMsg);
static MIB_ATTR_INDEX omci_GetFirstAttrIndex(omci_msg_attr_mask_t attrsSet);
static UINT32 omci_MibUploadGetOnRun(omci_msg_norm_baseline_t* pNorOmciMsg);

omci_msg_attr_mask_t omci_GetOltAccAttrSet(MIB_TABLE_INDEX tableIndex, omci_me_attr_access_mask_t accType);
GOS_ERROR_CODE omci_AttrsOmciSetToMibSet(omci_msg_attr_mask_t* pOmciAttrSet, MIB_ATTRS_SET* pMibAttrSet);

static void omci_dumpMulGetMsgInfo(UINT16 seqNum, UINT8 *pMsg, UINT32 size);

/*
 * Define Local Function
*/
static GOS_ERROR_CODE omci_InitFsm(void)
{
    static BOOL inited = FALSE;
    UINT32 i;

    if (inited)
    {
        return GOS_OK;
    }

    memset(gOmciFsmInfo, 0x00, OMCI_MAX_FSM_ID * sizeof(omci_fsm_info_ts));

    for (i = 0; i < OMCI_MAX_FSM_ID; i++)
    {
        gOmciFsmInfo[i].valid = FALSE;
    }

    inited = TRUE;

    return GOS_OK;
}


static omci_fsm_id omci_FsmCreate(UINT32 defaultState, UINT32 maxState, UINT32 maxEvent)
{
    omci_fsm_info_ts fsmInfo;
    UINT32     fsmId;

    memset(&fsmInfo, 0x00, sizeof(omci_fsm_info_ts));
    fsmInfo.curState = defaultState;
    fsmInfo.maxEvent = maxEvent;
    fsmInfo.maxState = maxState;
    fsmInfo.pHandlers = (OMCI_FSMEVTHDL*)malloc(sizeof(OMCI_FSMEVTHDL) * maxState * maxEvent);
    GOS_ASSERT(fsmInfo.pHandlers != NULL);
    memset(fsmInfo.pHandlers, 0x00, sizeof(OMCI_FSMEVTHDL));

    for (fsmId = 0; fsmId < OMCI_MAX_FSM_ID; fsmId++)
    {
        if (!gOmciFsmInfo[fsmId].valid)
        {
            break;
        }
    }

    if (fsmId < OMCI_MAX_FSM_ID)
    {
        memcpy(&(gOmciFsmInfo[fsmId]), &fsmInfo, sizeof(omci_fsm_info_ts));
        gOmciFsmInfo[fsmId].valid = TRUE;
    }

    return fsmId;
}

static GOS_ERROR_CODE omci_AddFsmEventHandler(omci_fsm_id fsmId, UINT32 state, UINT32 event, OMCI_FSMEVTHDL handler)
{
    omci_fsm_info_ts* pFsmInfo;


    GOS_ASSERT(fsmId < OMCI_MAX_FSM_ID);

    if (gOmciFsmInfo[fsmId].valid == TRUE)
    {
        pFsmInfo = &(gOmciFsmInfo[fsmId]);

        GOS_ASSERT(state < pFsmInfo->maxState);
        GOS_ASSERT(event < pFsmInfo->maxEvent);
        GOS_ASSERT(pFsmInfo->pHandlers);

        *(pFsmInfo->pHandlers + state * pFsmInfo->maxEvent + event) = handler;

        return GOS_OK;
    }

    return GOS_FAIL;
}

static GOS_ERROR_CODE omci_FsmRun(omci_fsm_id fsmId, UINT32 event, void* pEvtArg)
{
    omci_fsm_info_ts*    pFsmInfo;
    UINT32         newState;
    OMCI_FSMEVTHDL handler;


    GOS_ASSERT(fsmId < OMCI_MAX_FSM_ID);

    if (gOmciFsmInfo[fsmId].valid == TRUE)
    {
        pFsmInfo = &(gOmciFsmInfo[fsmId]);

        GOS_ASSERT(event < pFsmInfo->maxEvent);
        GOS_ASSERT(pFsmInfo->curState < pFsmInfo->maxState);
        newState = pFsmInfo->curState;


        handler = *(pFsmInfo->pHandlers + pFsmInfo->curState * pFsmInfo->maxEvent + event);
        if (handler)
        {
            newState = (*handler)(pEvtArg);
        }

        if (newState != pFsmInfo->curState)
        {
            pFsmInfo->curState = newState;
        }

        return GOS_OK;
    }

    return GOS_FAIL;
}

static void* omci_GetMibRowBuff(void)
{
    memset(&gOmciMibRowBuff[0], 0x00, MIB_TABLE_ENTRY_MAX_SIZE);
    return &gOmciMibRowBuff[0];
}

omci_msg_attr_mask_t omci_GetOltAccAttrSet(MIB_TABLE_INDEX tableIndex, omci_me_attr_access_mask_t accType)
{
    omci_msg_attr_mask_t    attrsSet;
    MIB_ATTRS_SET           mibAttrsSet;
    MIB_ATTR_INDEX          attrIndex;
    UINT32                  i;

    MIB_ClearAttrSet(&mibAttrsSet);

    // skip the first attribute, EntityID
    for (attrIndex = MIB_ATTR_NEXT_INDEX(MIB_ATTR_FIRST_INDEX), i = 1; i < MIB_GetTableAttrNum(tableIndex);
         i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        if (accType & MIB_GetAttrOltAcc(tableIndex, attrIndex))
        {
            MIB_SetAttrSet(&mibAttrsSet, attrIndex);
        }
    }

    OMCI_AttrsMibSetToOmciSet(&mibAttrsSet, &attrsSet);

    return attrsSet;
}


static GOS_ERROR_CODE omci_OmciDataToMibRow(MIB_TABLE_INDEX tableIndex, UINT8* pOmciData, void* pMibRow, omci_msg_attr_mask_t omciAttrSet)
{
    MIB_ATTRS_SET  mibAttrsSet;
    MIB_ATTR_INDEX attrIndex;
    UINT32         attrLen;
    MIB_ATTR_TYPE  attrType;
    UINT32         i;

    GOS_ASSERT(pOmciData);
    GOS_ASSERT(pMibRow);

    omci_AttrsOmciSetToMibSet(&omciAttrSet, &mibAttrsSet);

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
                    UINT8 value;
                    value = *pOmciData;
                    MIB_SetAttrToBuf(tableIndex, attrIndex, &value, pMibRow, attrLen);
                    break;
                }

                case MIB_ATTR_TYPE_UINT16:
                {
                    UINT16 value;
                    value = GOS_Ntohs(GOS_GetUINT16((UINT16*)pOmciData));
                    MIB_SetAttrToBuf(tableIndex, attrIndex, &value, pMibRow, attrLen);
                    break;
                }

                case MIB_ATTR_TYPE_UINT32:
                {
                    UINT32 value;
                    value = GOS_Ntohl(GOS_GetUINT32((UINT32*)pOmciData));
                    MIB_SetAttrToBuf(tableIndex, attrIndex, &value, pMibRow, attrLen);
                    break;
                }

                case MIB_ATTR_TYPE_UINT64:
                {
                    UINT64 value;
                    value = GOS_BuffToUINT64((CHAR*)pOmciData, attrLen);
                    MIB_SetAttrToBuf(tableIndex, attrIndex, &value, pMibRow, sizeof(UINT64));
                    break;
                }

                case MIB_ATTR_TYPE_STR:
                {
            MIB_SetAttrToBuf(tableIndex, attrIndex, pOmciData, pMibRow, attrLen+1);
                        break;
                }

                case MIB_ATTR_TYPE_TABLE:
                {
            MIB_SetAttrToBuf(tableIndex, attrIndex, pOmciData, pMibRow, attrLen);
                        break;
                }

                default:
                    break;
            }
            pOmciData  += attrLen;
        }
    }


    return GOS_OK;
}


GOS_ERROR_CODE omci_AttrsOmciSetToMibSet(omci_msg_attr_mask_t* pOmciAttrSet, MIB_ATTRS_SET* pMibAttrSet)
{
    MIB_ATTR_INDEX attrIndex;
    UINT32 i;

    MIB_ClearAttrSet(pMibAttrSet);

    for (attrIndex = MIB_ATTR_NEXT_INDEX(MIB_ATTR_FIRST_INDEX), i = 0; i < sizeof(omci_msg_attr_mask_t) * 8;
         i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        if ((*pOmciAttrSet) & (0x8000 >> i))
        {
            MIB_SetAttrSet(pMibAttrSet, attrIndex);
        }
    }

    return GOS_OK;
}


static GOS_ERROR_CODE omci_SyncMibData(void)
{
    MIB_TABLE_ONTDATA_T mibRow;

    memset(&mibRow, 0x00, sizeof(MIB_TABLE_ONTDATA_T));

    GOS_ASSERT(GOS_OK == MIB_GetFirst(MIB_TABLE_ONTDATA_INDEX, &mibRow, sizeof(MIB_TABLE_ONTDATA_T)));

    mibRow.MIBDataSync++;

    if (0 == mibRow.MIBDataSync)
    {
        mibRow.MIBDataSync = 1;
    }

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_ONTDATA_INDEX, &mibRow, sizeof(MIB_TABLE_ONTDATA_T)));

    OMCI_LOG(OMCI_LOG_LEVEL_INFO,"omci_SyncMibData: dataSync =0x%x", mibRow.MIBDataSync);

    return GOS_OK;
}


static GOS_ERROR_CODE omci_CreateRelatedMe(MIB_TABLE_INDEX tableIndex, void* pMibRow)
{
    /*
    if (MIB_TABLE_GEMIWTP_INDEX == tableIndex)
    {
        MIB_TABLE_GEMIWTP_T* pIwTp = (MIB_TABLE_GEMIWTP_T*)pMibRow;
        pIwTp->OpState = PON_ENABLE;
        MIB_Set(MIB_TABLE_GEMIWTP_INDEX, pIwTp, sizeof(MIB_TABLE_GEMIWTP_T));
    }

    if (MIB_TABLE_MULTIGEMIWTP_INDEX == tableIndex)
    {
        MIB_TABLE_MULTIGEMIWTP_T* pIwTp = (MIB_TABLE_MULTIGEMIWTP_T*)pMibRow;
        pIwTp->OpState = PON_ENABLE;
        MIB_Set(MIB_TABLE_MULTIGEMIWTP_INDEX, pIwTp, sizeof(MIB_TABLE_MULTIGEMIWTP_T));
    }
    */

    return GOS_OK;
}

static GOS_ERROR_CODE omci_SingleSet(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    void*                   pMibRowBuff     = omci_GetMibRowBuff();
    omci_me_class_t         classID         = (omci_me_class_t)pNorOmciMsg->meId.meClass;
    omci_me_instance_t      entityID        = (omci_me_instance_t)pNorOmciMsg->meId.meInstance;
    MIB_TABLE_INDEX         tableIndex      = MIB_GetTableIndexByClassId(classID);
    UINT32                  entrySize       = MIB_GetTableEntrySize(tableIndex);
    omci_msg_attr_mask_t    supportOmciSet  = omci_GetOptionAttrSet(tableIndex, OMCI_ME_ATTR_TYPE_OPTIONAL) | omci_GetOptionAttrSet(tableIndex, OMCI_ME_ATTR_TYPE_MANDATORY);
    omci_msg_attr_mask_t    writableOmciSet = omci_GetOltAccAttrSet(tableIndex, OMCI_ME_ATTR_ACCESS_WRITE);
    omci_msg_attr_mask_t    attrOmciSet;
    omci_msg_attr_mask_t    execOmciSet, validOmciSet;
    MIB_ATTRS_SET           validMibSet, unsupportOmciSet;
    CHAR                    oldMibRow[MIB_TABLE_ENTRY_MAX_SIZE];

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Handling SingleSet msg: class=%d, entity=0x%x", classID, entityID);

    // Entity ID is the first attribute
    MIB_SetAttrToBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &entityID, pMibRowBuff, sizeof(entityID));

    // Check if entity exists or not
    if (GOS_OK != MIB_Get(tableIndex, pMibRowBuff, entrySize))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"SingleSet: entity not exist in MIB, class=%d, entity=0x%x", classID, entityID);
        return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_UNKNOWN_ME_INSTANCE, TRUE);
    }

    memcpy(oldMibRow, pMibRowBuff, entrySize);

    attrOmciSet = (omci_msg_attr_mask_t)GOS_Ntohs(GOS_GetUINT16((UINT16*)&pNorOmciMsg->content[0]));

    omci_OmciDataToMibRow(tableIndex, &(pNorOmciMsg->content[2]), pMibRowBuff, attrOmciSet);

    // validSet indicats the valid attributes that can be set
    validOmciSet = attrOmciSet & supportOmciSet & writableOmciSet;
    omci_AttrsOmciSetToMibSet(&validOmciSet, &validMibSet);

    // Save in MIB
    if (GOS_OK != MIB_SetAttributes(tableIndex, pMibRowBuff, entrySize, &validMibSet))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"SingleSet: MIB_Set error, class=%d, entity=0x%x, attrSet=0x%x",
                  classID, entityID, validMibSet);
        return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
    }

    if (GOS_OK != MIB_Get(tableIndex, pMibRowBuff, entrySize))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"SingleSet: entity not exist in MIB, class=%d, entity=0x%x", classID, entityID);
        return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
    }

    if (GOS_OK != OMCI_MeOperCfg(tableIndex, oldMibRow, pMibRowBuff, MIB_SET, validMibSet, pNorOmciMsg->priority))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"SingleSet: SingleSet error, class=%d, entity=0x%x, attrSet=0x%x",
                  classID, entityID, validMibSet);
        // Rollback MIB
        MIB_Set(tableIndex, oldMibRow, entrySize);
        // Process error
        return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
    }


    omci_SyncMibData();

    // execSet indicats the attributes that can not be set but suppose to supporting set
    execOmciSet = validOmciSet;
    unsupportOmciSet = (~supportOmciSet) & attrOmciSet;

    // part of attributes executed, refer 983.2 P.335
    if (unsupportOmciSet)
    {
        memset(&pNorOmciMsg->content[0], 0x00, OMCI_MSG_BASELINE_CONTENTS_LEN);
        GOS_SetUINT16((UINT16*)&pNorOmciMsg->content[1], GOS_Htons((UINT16)unsupportOmciSet));
        GOS_SetUINT16((UINT16*)&pNorOmciMsg->content[3], GOS_Htons((UINT16)execOmciSet));
        OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_ATTR_FAILED_OR_UNKNOWN, FALSE);
    }
    else
    {
        // Success
        OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSED_SUCCESSFULLY, TRUE);
    }

    return GOS_OK;
}


static omci_msg_attr_mask_t omci_GetOptionAttrSet(MIB_TABLE_INDEX tableIndex, omci_me_attr_type_mask_t optType)
{
    omci_msg_attr_mask_t    attrsSet;
    MIB_ATTRS_SET           mibAttrsSet;
    MIB_ATTR_INDEX          attrIndex;
    UINT32                  i;

    MIB_ClearAttrSet(&mibAttrsSet);

    // skip the first attribute, EntityID
    for (attrIndex = MIB_ATTR_NEXT_INDEX(MIB_ATTR_FIRST_INDEX), i = 1; i < MIB_GetTableAttrNum(tableIndex);
         i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        if (optType & MIB_GetAttrOptionType(tableIndex, attrIndex))
        {
            MIB_SetAttrSet(&mibAttrsSet, attrIndex);
        }
    }

    OMCI_AttrsMibSetToOmciSet(&mibAttrsSet, &attrsSet);

    return attrsSet;
}

static GOS_ERROR_CODE omci_SingleGet(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    void*                   pMibRowBuff    = omci_GetMibRowBuff();
    omci_me_class_t         classID        = (omci_me_class_t)pNorOmciMsg->meId.meClass;
    omci_me_instance_t      entityID       = (omci_me_instance_t)pNorOmciMsg->meId.meInstance;
    MIB_TABLE_INDEX         tableIndex     = MIB_GetTableIndexByClassId(classID);
    UINT32                  entrySize      = MIB_GetTableEntrySize(tableIndex);
    omci_msg_attr_mask_t    supportOmciSet = omci_GetOptionAttrSet(tableIndex, OMCI_ME_ATTR_TYPE_OPTIONAL) | omci_GetOptionAttrSet(tableIndex, OMCI_ME_ATTR_TYPE_MANDATORY);
    omci_msg_attr_mask_t    attrOmciSet, validOmciSet, execOmciSet, succOmciSet;
    MIB_ATTRS_SET           validMibSet, succMibSet, unsupportOmciSet;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Handling SingleGet msg: class=%d, entity=0x%x", classID, entityID);

    // Entity ID is the first attribute
    MIB_SetAttrToBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &entityID, pMibRowBuff, sizeof(entityID));

    attrOmciSet = (omci_msg_attr_mask_t)GOS_Ntohs(GOS_GetUINT16((UINT16*)&pNorOmciMsg->content[0]));
    // validSet indicats the valid attributes that can be get
    validOmciSet = attrOmciSet & supportOmciSet;
    omci_AttrsOmciSetToMibSet(&validOmciSet, &validMibSet);

    // Check if entity exists or not
    if (OMCI_MSG_TYPE_GET == pNorOmciMsg->type)
    {
        if (GOS_OK != MIB_Get(tableIndex, pMibRowBuff, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_WARN,"SingleGet: entity not exist in MIB, class=%d, entity=0x%x", classID, entityID);
            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_UNKNOWN_ME_INSTANCE, TRUE);
        }
    }
    else
    {
        if (GOS_OK != MIB_GetCurrentData(tableIndex, pMibRowBuff, entrySize,entityID))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_WARN,"SingleGet: entity not exist in MIB, class=%d, entity=0x%x", classID, entityID);
            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_UNKNOWN_ME_INSTANCE, TRUE);
        }
    }
    // To inform driver to update MIB
    OMCI_MeOperCfg(tableIndex, pMibRowBuff, pMibRowBuff, MIB_GET, validMibSet, pNorOmciMsg->priority);

    memset(&pNorOmciMsg->content[0], 0x00, OMCI_MSG_BASELINE_CONTENTS_LEN);

    succMibSet = omci_MibRowToOmciData(tableIndex, pMibRowBuff, &pNorOmciMsg->content[3], &validMibSet);
    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"SingleGet: MibRowToOmciData, class=%d, entity=0x%x, validSet=0x%x, succSet=0x%x",
              classID, entityID, validMibSet, succMibSet);

    OMCI_AttrsMibSetToOmciSet(&succMibSet, &succOmciSet);

    // execSet indicats the attributes that can not be get but suppose to supporting get
    execOmciSet = validOmciSet & (~succOmciSet);
    unsupportOmciSet = (~supportOmciSet) & attrOmciSet;

    GOS_SetUINT16((UINT16*)&pNorOmciMsg->content[1], GOS_Htons((UINT16)succOmciSet));

    // part of attributes executed
    if (unsupportOmciSet)
    {
        GOS_SetUINT16((UINT16*)&pNorOmciMsg->content[OMCI_MSG_BASELINE_CONTENTS_LEN - 4], GOS_Htons((UINT16)unsupportOmciSet));
        GOS_SetUINT16((UINT16*)&pNorOmciMsg->content[OMCI_MSG_BASELINE_CONTENTS_LEN - 2], GOS_Htons((UINT16)execOmciSet));
        OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_ATTR_FAILED_OR_UNKNOWN, FALSE);
    }
    else
    {
        OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSED_SUCCESSFULLY, FALSE);
    }

    return GOS_OK;
}


static MIB_ATTRS_SET omci_MibRowToOmciData(MIB_TABLE_INDEX tableIndex, void* pMibRow, UINT8* pOmciData, MIB_ATTRS_SET* pMibAttrSet)
{
    MIB_ATTR_INDEX attrIndex;
    UINT32         attrLen;
    MIB_ATTR_TYPE  attrType;
    MIB_ATTRS_SET  resultSet;
    UINT32         doneSize;
    UINT32         i;

    GOS_ASSERT(pOmciData);
    GOS_ASSERT(pMibRow);

    MIB_ClearAttrSet(&resultSet);

    for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0, doneSize = 0; i < MIB_GetTableAttrNum(tableIndex);
         i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        if (MIB_IsInAttrSet(pMibAttrSet, attrIndex))
        {
            attrLen = MIB_GetAttrLen(tableIndex, attrIndex);
            attrType = MIB_GetAttrDataType(tableIndex, attrIndex);

            // exceed omci message size, just break;
            if ((attrLen + doneSize) > OMCI_MSG_BASELINE_GET_RSP_LIMIT)
            {
                break;
            }

            MIB_SetAttrSet(&resultSet, attrIndex);

            switch (attrType)
            {
                case MIB_ATTR_TYPE_UINT8:
                {
                    UINT8 value;
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &value, pMibRow, attrLen);
                    *pOmciData = value;
                    break;
                }

                case MIB_ATTR_TYPE_UINT16:
                {
                    UINT16 value;
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &value, pMibRow, attrLen);
                    GOS_SetUINT16((UINT16*)pOmciData, GOS_Htons(value));
                    break;
                }

                case MIB_ATTR_TYPE_UINT32:
                {
                    UINT32 value;
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &value, pMibRow, attrLen);
                    GOS_SetUINT32((UINT32*)pOmciData, GOS_Htonl(value));
                    break;
                }

                case MIB_ATTR_TYPE_UINT64:
                {
                    UINT64 value;
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &value, pMibRow, sizeof(UINT64));
                    value = GOS_Htonll(value);
                    GOS_UINT64ToBuff(value, (CHAR*)pOmciData, attrLen);
                    break;
                }

                case MIB_ATTR_TYPE_STR:
                {
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, temp, pMibRow, attrLen + 1);
                    memcpy(pOmciData, temp, attrLen);
                    break;
                }

                case MIB_ATTR_TYPE_TABLE:
                {
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, temp, pMibRow, attrLen);
                    memcpy(pOmciData, temp, attrLen);
                    break;
                }

                default:
                    break;
            }
            pOmciData  += attrLen;
            doneSize   += attrLen;
        }
    }

    return resultSet;
}

static GOS_ERROR_CODE omci_MultiGet(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    UINT32 pri = pNorOmciMsg->priority;

    GOS_ASSERT(pri < OMCI_MSG_BASELINE_PRI_NUM);

    omci_FsmRun(gOmciMulGetFsm[pri], OMCI_MULGET_EVT_START, pNorOmciMsg);

    return GOS_OK;
}

static UINT32 omci_MulGetStartOnIdle(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    void*                   pMibRowBuff   = omci_GetMibRowBuff();
    UINT32                  pri           = pNorOmciMsg->priority;
    omci_me_class_t         classID       = (omci_me_class_t)pNorOmciMsg->meId.meClass;
    omci_me_instance_t      entityID      = (omci_me_instance_t)pNorOmciMsg->meId.meInstance;
    MIB_TABLE_INDEX         tableIndex    = MIB_GetTableIndexByClassId(classID);
    omci_msg_attr_mask_t    supportOmciSet = omci_GetOptionAttrSet(tableIndex, OMCI_ME_ATTR_TYPE_OPTIONAL) | omci_GetOptionAttrSet(tableIndex, OMCI_ME_ATTR_TYPE_MANDATORY);
    omci_msg_attr_mask_t    attrOmciSet   = GOS_Ntohs(GOS_GetUINT16((UINT16*)&pNorOmciMsg->content[0]));
    UINT8*                  pOmciData     = &pNorOmciMsg->content[3];
    omci_msg_attr_mask_t    validOmciSet, execOmciSet, succOmciSet;
    MIB_ATTRS_SET           validMibSet, unsupportOmciSet, succMibSet = 0;
    MIB_ATTR_INDEX          attrIndex;
    UINT32                  i;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO,"Start Getting Large Attributes: class=%d, entity=0x%x", classID, entityID);

    // Entity ID is the first attribute
    MIB_SetAttrToBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &entityID, pMibRowBuff, sizeof(entityID));

    validOmciSet = attrOmciSet & supportOmciSet;
    omci_AttrsOmciSetToMibSet(&validOmciSet, &validMibSet);

    // Check if entity exists or not
    if (OMCI_MSG_TYPE_GET == pNorOmciMsg->type)
    {
        if (GOS_OK != MIB_Get(tableIndex, pMibRowBuff, MIB_GetTableEntrySize(tableIndex)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_WARN,"MulGet: entity not exist in MIB, class=%d, entity=0x%x", classID, entityID);
            OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_UNKNOWN_ME_INSTANCE, TRUE);
            return OMCI_MULGET_STATE_IDLE;
        }
    } else
    {
        if (GOS_OK != MIB_GetCurrentData(tableIndex, pMibRowBuff, MIB_GetTableEntrySize(tableIndex),entityID))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_WARN,"SingleGet: entity not exist in MIB, class=%d, entity=0x%x", classID, entityID);
            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_UNKNOWN_ME_INSTANCE, TRUE);
        }
    }

    // To inform driver to update MIB
    OMCI_MeOperCfg(tableIndex, pMibRowBuff, pMibRowBuff, MIB_GET, validMibSet, pNorOmciMsg->priority);

    memset(&pNorOmciMsg->content[0], 0x00, OMCI_MSG_BASELINE_CONTENTS_LEN);

    for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0; i < MIB_GetTableAttrNum(tableIndex);
         i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        if (MIB_IsInAttrSet(&validMibSet, attrIndex))
        {
            MIB_ATTR_TYPE attrType;
            UINT32 attrLen;

            attrLen = MIB_GetAttrLen(tableIndex, attrIndex);
            attrType = MIB_GetAttrDataType(tableIndex, attrIndex);

            // table or large attribute
            if ((attrType == MIB_ATTR_TYPE_TABLE)||(attrLen > OMCI_MSG_BASELINE_GET_RSP_LIMIT))
            {
                attrLen = 4;
            }

            MIB_SetAttrSet(&succMibSet, attrIndex);

            switch (attrType)
            {
                case MIB_ATTR_TYPE_UINT8:
                {
                    UINT8 value;
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &value, pMibRowBuff, attrLen);
                    *pOmciData = value;
                    break;
                }

                case MIB_ATTR_TYPE_UINT16:
                {
                    UINT16 value;
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &value, pMibRowBuff, attrLen);
                    GOS_SetUINT16((UINT16*)pOmciData, GOS_Htons(value));
                    break;
                }

                case MIB_ATTR_TYPE_UINT32:
                {
                    UINT32 value;
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &value, pMibRowBuff, attrLen);
                    GOS_SetUINT32((UINT32*)pOmciData, GOS_Htonl(value));
                    break;
                }

                case MIB_ATTR_TYPE_UINT64:
                {
                    UINT64 value;
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &value, pMibRowBuff, sizeof(UINT64));
                    value = GOS_Htonll(value);
                    GOS_UINT64ToBuff(value, (CHAR*)pOmciData, attrLen);
                    break;
                }

                case MIB_ATTR_TYPE_STR:
                {
                    if (MIB_GetAttrLen(tableIndex, attrIndex) > OMCI_MSG_BASELINE_GET_RSP_LIMIT)
                    {
                        MIB_GetAttrFromBuf(tableIndex, attrIndex, gOmciMulGetData[pri].attribute[attrIndex].attrValue, pMibRowBuff, MIB_GetAttrLen(tableIndex, attrIndex) + 1);
                        gOmciMulGetData[pri].attribute[attrIndex].attrSize = MIB_GetAttrLen(tableIndex, attrIndex);
                        GOS_SetUINT32((UINT32*)pOmciData, GOS_Htonl(MIB_GetAttrLen(tableIndex, attrIndex)));

                        gOmciMulGetData[pri].attribute[attrIndex].attrIndex = attrIndex;
                        gOmciMulGetData[pri].attribute[attrIndex].doneSeqNum = 0;
                        gOmciMulGetData[pri].attribute[attrIndex].maxSeqNum = (gOmciMulGetData[pri].attribute[attrIndex].attrSize + OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT - 1) / OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT;
                    }
                    else
                    {
                        MIB_GetAttrFromBuf(tableIndex, attrIndex, mulGetTempBuff, pMibRowBuff, attrLen + 1);
                        memcpy(pOmciData, mulGetTempBuff, attrLen);
                    }
                    break;
                }

                case MIB_ATTR_TYPE_TABLE:
                {
                    /*
                    MIB_TABLE_INDEX     subTableIndex;
                    GOS_ERROR_CODE      ret;
                    omci_me_instance_t  entryEntityId;
                    UINT32              offset = 0;

                    subTableIndex = MIB_GetTableIndexByName(MIB_GetAttrName(tableIndex, attrIndex));
                    if (MIB_TABLE_INDEX_VALID(subTableIndex))
                    {
                        ret = MIB_GetFirst(subTableIndex, pMibRowBuff, MIB_GetTableEntrySize(subTableIndex));
                        while (GOS_OK == ret)
                        {
                            MIB_GetAttrFromBuf(subTableIndex, MIB_ATTR_FIRST_INDEX, &entryEntityId, pMibRowBuff, sizeof(UINT16));
                            if (entryEntityId == entityID)
                            {
                                offset += omci_GetTableAttrLen(tableIndex, attrIndex);
                            }

                            ret = MIB_GetNext(subTableIndex, pMibRowBuff, MIB_GetTableEntrySize(subTableIndex));
                        }
                    }

                    gOmciMulGetData[pri].attribute[attrIndex].attrSize = offset;
                    GOS_SetUINT32((UINT32*)pOmciData, GOS_Htonl(offset));

                    gOmciMulGetData[pri].attribute[attrIndex].attrIndex = attrIndex;
                    gOmciMulGetData[pri].attribute[attrIndex].doneSeqNum = 0;
                    gOmciMulGetData[pri].attribute[attrIndex].maxSeqNum = (gOmciMulGetData[pri].attribute[attrIndex].attrSize + OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT - 1) / OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT;
                    */

                    // OmciMulGetData[pri].attribute[attrIndex].attrSize is total size of this table attribute
                    GOS_SetUINT32((UINT32*)pOmciData, GOS_Htonl(gOmciMulGetData[pri].attribute[attrIndex].attrSize));
                    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "table attribute size %u", gOmciMulGetData[pri].attribute[attrIndex].attrSize);
                    break;
                }

                default:
                    break;
            }

            pOmciData  += attrLen;
        }
    }

    gOmciMulGetData[pri].classID = classID;
    gOmciMulGetData[pri].entityID = entityID;

    OMCI_AttrsMibSetToOmciSet(&succMibSet, &succOmciSet);

    // execSet indicats the attributes that can not be get but suppose to supporting get
    execOmciSet = validOmciSet & (~succOmciSet);
    unsupportOmciSet = (~supportOmciSet) & attrOmciSet;

    GOS_SetUINT16((UINT16*)&pNorOmciMsg->content[1], GOS_Htons((UINT16)succOmciSet));

    // part of attributes executed
    if (unsupportOmciSet)
    {
        GOS_SetUINT16((UINT16*)&pNorOmciMsg->content[OMCI_MSG_BASELINE_CONTENTS_LEN - 4], GOS_Htons((UINT16)unsupportOmciSet));
        GOS_SetUINT16((UINT16*)&pNorOmciMsg->content[OMCI_MSG_BASELINE_CONTENTS_LEN - 2], GOS_Htons((UINT16)execOmciSet));
        OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_ATTR_FAILED_OR_UNKNOWN, FALSE);
    }
    else
    {
        OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSED_SUCCESSFULLY, FALSE);
    }

    return OMCI_MULGET_STATE_RUN;
}

#if 0
static UINT32 omci_GetTableAttrLen(MIB_TABLE_INDEX tableIndex, MIB_ATTR_INDEX attrIndex)
{
    /*
    // here MIB_GetAttrLen(tableIndex, attrIndex) is 12 * 2, :(
    if ((MIB_TABLE_MULTIGEMIWTP_INDEX == tableIndex) && \
        (MIB_TABLE_MULTIGEMIWTP_MCASTADDRTABLE_INDEX == attrIndex))
    {
        return 12;
    }
    */

    return MIB_GetAttrLen(tableIndex, attrIndex);
}
#endif

static UINT32 omci_MulGetGetOnIdle(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PARAMETER_ERROR, TRUE);
    OMCI_LOG(OMCI_LOG_LEVEL_INFO,"Get on IDLE state of Getting Large Attribute");
    return OMCI_MULGET_STATE_IDLE;
}


static UINT32 omci_MulGetStartOnRun(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    UINT32 pri = pNorOmciMsg->priority;
    omci_msg_attr_mask_t attrOmciSet = GOS_Ntohs(GOS_GetUINT16((UINT16*)&pNorOmciMsg->content[0]));
    MIB_ATTR_INDEX     attrIndex = omci_GetFirstAttrIndex(attrOmciSet);

    OMCI_LOG(OMCI_LOG_LEVEL_WARN,"Terminate(interrupt) Getting Large Attribute: class=%d, entity=0x%x, attr=%d",
              gOmciMulGetData[pri].classID, gOmciMulGetData[pri].entityID, attrIndex);

    // Do same thing as startOnIdle
    return omci_MulGetStartOnIdle(pNorOmciMsg);
}

static void omci_dumpMulGetMsgInfo(UINT16 seqNum, UINT8 *pMsg, UINT32 size)
{
    UINT8 *ptr = pMsg, i;
    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "[%u]---------------------------", seqNum);
    for (i = 0; i < size; i++)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "pMsg[%u]=%02x", i, *(ptr+i));
    }
    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "------------------------------");
    return;
}

static UINT32 omci_MulGetGetOnRun(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    UINT32                  pri = pNorOmciMsg->priority;
    omci_me_class_t         classID = (omci_me_class_t)pNorOmciMsg->meId.meClass;
    omci_me_instance_t      entityID = (omci_me_instance_t)pNorOmciMsg->meId.meInstance;
    omci_msg_attr_mask_t    attrOmciSet = GOS_Ntohs(GOS_GetUINT16((UINT16*)&pNorOmciMsg->content[0]));
    MIB_ATTR_INDEX          attrIndex = omci_GetFirstAttrIndex(attrOmciSet);
    UINT16                  seqNum = GOS_Ntohs(GOS_GetUINT16((UINT16*)&pNorOmciMsg->content[2]));
    UINT32                  curSize;
    UINT32                  offset;

    if ((gOmciMulGetData[pri].classID != classID) ||
        (gOmciMulGetData[pri].entityID != entityID) ||
        (gOmciMulGetData[pri].attribute[attrIndex].attrIndex != attrIndex))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Received para is wrong: pri=%d, class=%d, entity=0x%x, attr=%d",
                              pri, classID, entityID, attrIndex);
        OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PARAMETER_ERROR, TRUE);
        return OMCI_MULGET_STATE_RUN;
    }

    // Sequence number is out of range
    if (seqNum >= gOmciMulGetData[pri].attribute[attrIndex].maxSeqNum)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Received sequence is out of range: seq=%d,max=%d",
                  seqNum, gOmciMulGetData[pri].attribute[attrIndex].maxSeqNum);
        OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PARAMETER_ERROR, TRUE);
        return OMCI_MULGET_STATE_RUN;
    }

    curSize = OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT;
    // if it is the last sequence
    if (seqNum == gOmciMulGetData[pri].attribute[attrIndex].maxSeqNum - 1)
    {
        curSize = gOmciMulGetData[pri].attribute[attrIndex].attrSize - OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT * seqNum;
    }

    offset = OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT * seqNum;

    GOS_SetUINT16((UINT16*)&pNorOmciMsg->content[1], GOS_Htons(attrOmciSet));
    memcpy(&pNorOmciMsg->content[3], gOmciMulGetData[pri].attribute[attrIndex].attrValue + offset, curSize);
    omci_dumpMulGetMsgInfo(seqNum, &pNorOmciMsg->content[3], curSize);
    OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSED_SUCCESSFULLY, FALSE);


    gOmciMulGetData[pri].attribute[attrIndex].doneSeqNum++;

    // Done
    if (gOmciMulGetData[pri].attribute[attrIndex].doneSeqNum >= gOmciMulGetData[pri].attribute[attrIndex].maxSeqNum)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,"Terminate(done) Getting Large Attribute: class=%d, entity=0x%x, attr=%d",
                 gOmciMulGetData[pri].classID, gOmciMulGetData[pri].entityID, attrIndex);
        return OMCI_MULGET_STATE_IDLE;
    }

    return OMCI_MULGET_STATE_RUN;
}


static UINT32 omci_MibUploadStartOnIdle(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    UINT16 maxSeq;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO,"Start MIB Upload");

    if (GOS_OK != MIB_CreatePublicTblSnapshot())
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"MIB_CreatePublicTblSnapshot returned failure");
        OMCI_ResponseMsg(pNorOmciMsg, (omci_msg_response_t)0, TRUE);
        return OMCI_MIB_UPLOAD_STATE_IDLE;
    }

    maxSeq = omci_GetMibUploadMaxSeqNum();

    // MIB is empty
    if (0 == maxSeq)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,"MIB is empty");
        MIB_DeletePublicTblSnapshot();
        OMCI_ResponseMsg(pNorOmciMsg, (omci_msg_response_t)0, TRUE);
        return OMCI_MIB_UPLOAD_STATE_IDLE;
    }

    memset(&pNorOmciMsg->content[0], 0x00, OMCI_MSG_BASELINE_CONTENTS_LEN);
    GOS_SetUINT16((UINT16*)&pNorOmciMsg->content[0], GOS_Htons(maxSeq));
    OMCI_ResponseMsg(pNorOmciMsg, (omci_msg_response_t)pNorOmciMsg->content[0], FALSE);

    gOmciMibUploadData.curChannelPri = pNorOmciMsg->priority;
    gOmciMibUploadData.doneSeqNum    = 0;
    gOmciMibUploadData.curTableindex = 0;
    gOmciMibUploadData.tmpSeqNum     = 0;
    gOmciMibUploadData.maxSeqNum     = maxSeq - 1;

    return OMCI_MIB_UPLOAD_STATE_RUN;
}


static UINT16 omci_GetMibUploadMaxSeqNum(void)
{
    UINT16 numOfSubseqCom = 0;
    UINT16 i = 0;
    MIB_TABLE_INDEX tableIndex;

    for(i = 0, tableIndex = MIB_TABLE_FIRST_INDEX; i < MIB_TABLE_TOTAL_NUMBER;
        i++, tableIndex = MIB_TABLE_NEXT_INDEX(tableIndex))
    {
        if ((MIB_GetTableStdType(tableIndex) & OMCI_ME_TYPE_PRIVATE)
            || (MIB_GetTableStdType(tableIndex) & OMCI_ME_TYPE_NOT_MIB_UPLOAD))
        {
            continue;
        }
        numOfSubseqCom += MIB_GetSnapshotEntryCount(tableIndex) * omci_GetSeqNumPerRow(tableIndex);
    }
    return numOfSubseqCom;
}

static UINT16 omci_GetSeqNumPerRow(MIB_TABLE_INDEX tableIndex)
{
    UINT16 numOfAttr      = MIB_GetTableAttrNum(tableIndex);
    UINT16 aggreAttrLen   = 0;
    UINT16 numOfSubseqCom = 0;
    UINT16 attrLen;
    omci_me_attr_type_mask_t attrOption;
    UINT16 i;

    // skip the first attribute (EntityID)
    MIB_ATTR_INDEX attrIndex = MIB_ATTR_NEXT_INDEX(MIB_ATTR_FIRST_INDEX);
    for (i = 1; i < numOfAttr; i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        attrOption = MIB_GetAttrOptionType(tableIndex, attrIndex);

        // skip unwanted attributes (not support, private, pm & table)
        if ((OMCI_ME_ATTR_TYPE_M_NOT_SUPPORT & attrOption)
                || (OMCI_ME_ATTR_TYPE_O_NOT_SUPPORT & attrOption)
                || (OMCI_ME_ATTR_TYPE_PRIVATE & attrOption)
                || (OMCI_ME_ATTR_TYPE_PM & attrOption)
                || (OMCI_ME_ATTR_TYPE_TABLE & attrOption))
        {
            continue;
        }

        attrLen = MIB_GetAttrLen(tableIndex, attrIndex);

        // if the size of this attribute is larger than the OMCI message content size, just skip it
        if (OMCI_MSG_BASELINE_MIB_UPLOAD_NEXT_RSP_LIMIT < attrLen)
        {
            continue;
        }

        // this attribute will be in a new message
        if ((aggreAttrLen + attrLen) > OMCI_MSG_BASELINE_MIB_UPLOAD_NEXT_RSP_LIMIT)
        {
            aggreAttrLen = attrLen;
            numOfSubseqCom++;
        }
        else
        {
            aggreAttrLen += attrLen;
        }
    }


    if (aggreAttrLen > 0)
    {
        numOfSubseqCom++;
    }

    // the ME has no other attributes exept entityID
    if (0 == numOfSubseqCom)
    {
        numOfSubseqCom = 1;
    }

    return numOfSubseqCom;
}

static GOS_ERROR_CODE
omci_GetMibUploadData (
    UINT16 seqNum,
    CHAR*  pDataBuf,
    UINT32           size,
    MIB_TABLE_INDEX* pTableIndex,
    omci_me_instance_t* pEntityId,
    MIB_ATTRS_SET*      pMibAttrSet )
{
    UINT16 curTableSeqNum;
    UINT8*  rowBuff = omci_GetMibRowBuff();
    UINT8*  attrBuff;
    UINT16  j;
    UINT16  entityID = 0xFFFF;

    MIB_ATTR_INDEX  attrIndex;
    UINT16          attrLen;
    omci_me_attr_type_mask_t attrOption;
    MIB_ATTR_TYPE   attrType;
    MIB_ATTRS_SET   attrSet;
    UINT32          totalAttrSize;
    UINT32          maxSeqNumPerRow;
    UINT32          meType;

    GOS_ASSERT(pDataBuf && pTableIndex && pEntityId);

    if ( gOmciMibUploadData.curTableindex == 0)
    {
        //first mib upload, just upload the first ME in the list.
        gOmciMibUploadData.curTableindex = MIB_GetFirstTableindex();
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"First tableindex:%d", gOmciMibUploadData.curTableindex);
    }
    else if ( gOmciMibUploadData.doneSeqNum - gOmciMibUploadData.tmpSeqNum == 0)
    {
        //the table is upload completely, upload the next ME.
        MIB_GetNextClassIdTableIndex(&(gOmciMibUploadData.curTableindex));
    }

    //Skip the ME what is no need to upload.
    while (1)
    {
        meType = MIB_GetTableStdType(gOmciMibUploadData.curTableindex);
        if ((meType & OMCI_ME_TYPE_PRIVATE) || (meType & OMCI_ME_TYPE_NOT_MIB_UPLOAD))
        {
            MIB_GetNextClassIdTableIndex(&(gOmciMibUploadData.curTableindex));
            continue;
        }

        maxSeqNumPerRow = omci_GetSeqNumPerRow(gOmciMibUploadData.curTableindex);
        if ((curTableSeqNum = MIB_GetSnapshotEntryCount(gOmciMibUploadData.curTableindex) * maxSeqNumPerRow) == 0)
        {
            MIB_GetNextClassIdTableIndex(&(gOmciMibUploadData.curTableindex));
            continue;
        }

        break;
    }

    if (seqNum >= gOmciMibUploadData.tmpSeqNum)
    {
        gOmciMibUploadData.tmpSeqNum += curTableSeqNum;
    }

    seqNum = curTableSeqNum - (gOmciMibUploadData.tmpSeqNum - seqNum);
    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"ME_UPLOAD tmpSeqNum:%d, seqNum:%d, curTableindex:%d\n", gOmciMibUploadData.tmpSeqNum, seqNum, gOmciMibUploadData.curTableindex);

    // this table is selected
    if (GOS_OK == MIB_GetSnapshotFirst(gOmciMibUploadData.curTableindex, rowBuff, MIB_GetTableEntrySize(gOmciMibUploadData.curTableindex)))
    {
        do
        {
            // this entity is selected
            if (seqNum < maxSeqNumPerRow)
            {
                MIB_GetAttrFromBuf(gOmciMibUploadData.curTableindex, MIB_ATTR_FIRST_INDEX, &entityID, rowBuff, sizeof(UINT16));
                break;
            }
            seqNum -= maxSeqNumPerRow;
        }while (GOS_OK == MIB_GetSnapshotNext(gOmciMibUploadData.curTableindex, rowBuff, MIB_GetTableEntrySize(gOmciMibUploadData.curTableindex)));
    }

    if (entityID == 0xFFFF) {
        return GOS_ERR_NOT_FOUND;
    }

    // Table and entity are targeted
    *pTableIndex = gOmciMibUploadData.curTableindex;
    *pEntityId = entityID;
    // Search the beginning attribute
    attrIndex = MIB_ATTR_NEXT_INDEX(MIB_ATTR_FIRST_INDEX);
    totalAttrSize = 0;
    for (j = 1; j < MIB_GetTableAttrNum(gOmciMibUploadData.curTableindex); j++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        if (0 == seqNum)
        {
            break;
        }

        attrOption = MIB_GetAttrOptionType(gOmciMibUploadData.curTableindex, attrIndex);

        // skip unwanted attributes (not support, private, pm & table)
        if ((OMCI_ME_ATTR_TYPE_M_NOT_SUPPORT & attrOption)
            || (OMCI_ME_ATTR_TYPE_O_NOT_SUPPORT & attrOption)
            || (OMCI_ME_ATTR_TYPE_PRIVATE & attrOption)
            || (OMCI_ME_ATTR_TYPE_PM & attrOption)
            || (OMCI_ME_ATTR_TYPE_TABLE & attrOption))
        {
            continue;
        }

        attrLen = MIB_GetAttrLen(gOmciMibUploadData.curTableindex, attrIndex);
        if ((totalAttrSize + attrLen) > OMCI_MSG_BASELINE_MIB_UPLOAD_NEXT_RSP_LIMIT)
        {
            seqNum--;
            totalAttrSize = 0;
            if (0 == seqNum)
            {
                break;
            }
        }
        totalAttrSize += attrLen;
    }

    MIB_ClearAttrSet(&attrSet);
    // Attribute is selected
    for (; j < MIB_GetTableAttrNum(gOmciMibUploadData.curTableindex); j++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        attrOption = MIB_GetAttrOptionType(gOmciMibUploadData.curTableindex, attrIndex);
        attrType = MIB_GetAttrDataType(gOmciMibUploadData.curTableindex, attrIndex);

        // skip unwanted attributes (not support, private, pm & table)
        if ((OMCI_ME_ATTR_TYPE_M_NOT_SUPPORT & attrOption)
            || (OMCI_ME_ATTR_TYPE_O_NOT_SUPPORT & attrOption)
            || (OMCI_ME_ATTR_TYPE_PRIVATE & attrOption)
            || (OMCI_ME_ATTR_TYPE_PM & attrOption)
            || (OMCI_ME_ATTR_TYPE_TABLE & attrOption))
        {
            continue;
        }

        attrLen = MIB_GetAttrLen(gOmciMibUploadData.curTableindex, attrIndex);
        // if the size of this attribute is larger than the OMCI message content size, just skip it
        if (OMCI_MSG_BASELINE_MIB_UPLOAD_NEXT_RSP_LIMIT < attrLen)
        {
            continue;
        }

        // the left space in not enough to adopt this attribute, then stop
        if (attrLen > size)
        {
            break;
        }

        // OK, copy the attribute value into the buffer
        switch (attrType)
        {
            case MIB_ATTR_TYPE_UINT16:
            {
                UINT16 value;

                MIB_GetAttrFromBuf(gOmciMibUploadData.curTableindex, attrIndex, &value, rowBuff, attrLen);
                value = GOS_Htons(value);
                memcpy(pDataBuf, &value, attrLen);
                break;
            }

            case MIB_ATTR_TYPE_UINT32:
            {
                UINT32 value;

                MIB_GetAttrFromBuf(gOmciMibUploadData.curTableindex, attrIndex, &value, rowBuff, attrLen);
                value = GOS_Htonl(value);
                memcpy(pDataBuf, &value, attrLen);
                break;
            }
            case MIB_ATTR_TYPE_UINT64:
            {
                UINT64 value;

                MIB_GetAttrFromBuf(gOmciMibUploadData.curTableindex, attrIndex, &value, rowBuff, sizeof(UINT64));
                GOS_UINT64ToBuff(value, pDataBuf, attrLen);
                break;
            }
            case MIB_ATTR_TYPE_UINT8:
            {
                UINT8 value;

                MIB_GetAttrFromBuf(gOmciMibUploadData.curTableindex, attrIndex, &value, rowBuff, attrLen);
                memcpy(pDataBuf, &value, attrLen);
                break;
            }
            case MIB_ATTR_TYPE_STR:
            {
                attrBuff = malloc(MIB_TABLE_ATTR_MAX_SIZE);
                if (!attrBuff)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Allocate memory for attrBuff fail: %s", __FUNCTION__);
                    return GOS_FAIL;
                }

                MIB_GetAttrFromBuf(gOmciMibUploadData.curTableindex, attrIndex, attrBuff, rowBuff, attrLen + 1);
                memcpy(pDataBuf, attrBuff, attrLen);
                free(attrBuff);
                break;
            }
            default:
                break;
        }
        MIB_SetAttrSet(&attrSet, attrIndex);
        pDataBuf += attrLen;
        size -= attrLen;
    }

    if (pMibAttrSet)
    {
        *pMibAttrSet = attrSet;
        return GOS_OK;
    } else {
        return GOS_FAIL;
    }

}


static UINT32 omci_MibUploadGetOnIdle(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    OMCI_ResponseMsg(pNorOmciMsg, (omci_msg_response_t)0, TRUE);
    OMCI_LOG(OMCI_LOG_LEVEL_WARN,"Get on IDLE state of MIB Upload");
    return OMCI_MIB_UPLOAD_STATE_IDLE;
}


static UINT32 omci_MibUploadStartOnRun(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    MIB_DeletePublicTblSnapshot();
    OMCI_LOG(OMCI_LOG_LEVEL_WARN,"Terminate(interrupt) MIB Upload");

    return omci_MibUploadStartOnIdle(pNorOmciMsg);
}


static MIB_ATTR_INDEX omci_GetFirstAttrIndex(omci_msg_attr_mask_t attrsSet)
{
    MIB_ATTRS_SET  attrMibSet;
    MIB_ATTR_INDEX attrIndex;
    UINT32         i;

    omci_AttrsOmciSetToMibSet(&attrsSet, &attrMibSet);


    for (attrIndex = MIB_ATTR_NEXT_INDEX(MIB_ATTR_FIRST_INDEX), i = 0; i < sizeof(omci_msg_attr_mask_t) * 8;
         i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        if (MIB_IsInAttrSet(&attrMibSet, attrIndex))
        {
            return attrIndex;
        }
    }

    return MIB_ATTR_UNKNOWN_INDEX;
}


static UINT32 omci_MibUploadGetOnRun(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    GOS_ERROR_CODE ret = GOS_FAIL;
    UINT32                  seqNum = GOS_Ntohs(GOS_GetUINT16((UINT16*)&pNorOmciMsg->content[0]));
    MIB_TABLE_INDEX         tableIndex = 0;
    omci_me_instance_t      entityID = 0;
    MIB_ATTRS_SET           attrSet;
    omci_msg_attr_mask_t    omciSet;

    // Check channel
    if (gOmciMibUploadData.curChannelPri != pNorOmciMsg->priority)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,"Receive MibUploadNext on wrong channel: expected=%d, received=%d",
                  gOmciMibUploadData.curChannelPri, pNorOmciMsg->priority);
        OMCI_ResponseMsg(pNorOmciMsg, (omci_msg_response_t)0, TRUE);
        return OMCI_MIB_UPLOAD_STATE_RUN;
    }

    // Check sequence
    if (seqNum > gOmciMibUploadData.maxSeqNum)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,"Received seqnum is out of range: seq=%d, max=%d",
                  seqNum, gOmciMibUploadData.maxSeqNum);
        OMCI_ResponseMsg(pNorOmciMsg, (omci_msg_response_t)0, TRUE);
        return OMCI_MIB_UPLOAD_STATE_RUN;
    }

    // Get MIB Data
    ret = omci_GetMibUploadData(
                seqNum,
                (CHAR*)&(pNorOmciMsg->content[6]),
                OMCI_MSG_BASELINE_MIB_UPLOAD_NEXT_RSP_LIMIT,
                &tableIndex,
                &entityID,
                &attrSet);
    if (ret != GOS_OK)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Failed to get the mib data for MibUpload");
    }

    OMCI_AttrsMibSetToOmciSet(&attrSet, &omciSet);
    GOS_SetUINT16((UINT16*)&pNorOmciMsg->content[0], GOS_Htons((UINT16)MIB_GetTableClassId(tableIndex)));
    GOS_SetUINT16((UINT16*)&pNorOmciMsg->content[2], GOS_Htons((UINT16)entityID));
    GOS_SetUINT16((UINT16*)&pNorOmciMsg->content[4], GOS_Htons((UINT16)omciSet));

    OMCI_ResponseMsg(pNorOmciMsg, (omci_msg_response_t)pNorOmciMsg->content[0], FALSE);

    gOmciMibUploadData.doneSeqNum++;

    // done
    if (gOmciMibUploadData.doneSeqNum >= gOmciMibUploadData.maxSeqNum + 1)
    {
        MIB_DeletePublicTblSnapshot();
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,"Terminate(done) MIB Upload");

        return OMCI_MIB_UPLOAD_STATE_IDLE;
    }

    return OMCI_MIB_UPLOAD_STATE_RUN;
}



/*
 * Define Global Function for OMCI State Machine
*/

GOS_ERROR_CODE OMCI_InitStateMachines(void)
{
    omci_InitFsm();

    /* Multi Get state machine */
    memset(gOmciMulGetData, 0x00, OMCI_MSG_BASELINE_PRI_NUM * sizeof(omci_mulget_info_ts));

    gOmciMulGetFsm[OMCI_MSG_BASELINE_PRI_LOW] = omci_FsmCreate(OMCI_MULGET_STATE_IDLE, OMCI_MULGET_STATE_NUM, OMCI_MULGET_EVT_NUM);
    omci_AddFsmEventHandler(gOmciMulGetFsm[OMCI_MSG_BASELINE_PRI_LOW], OMCI_MULGET_STATE_IDLE, \
                        OMCI_MULGET_EVT_START,  (OMCI_FSMEVTHDL)omci_MulGetStartOnIdle);
    omci_AddFsmEventHandler(gOmciMulGetFsm[OMCI_MSG_BASELINE_PRI_LOW], OMCI_MULGET_STATE_IDLE, \
                        OMCI_MULGET_EVT_GET,    (OMCI_FSMEVTHDL)omci_MulGetGetOnIdle);
    omci_AddFsmEventHandler(gOmciMulGetFsm[OMCI_MSG_BASELINE_PRI_LOW], OMCI_MULGET_STATE_RUN,  \
                        OMCI_MULGET_EVT_START,  (OMCI_FSMEVTHDL)omci_MulGetStartOnRun);
    omci_AddFsmEventHandler(gOmciMulGetFsm[OMCI_MSG_BASELINE_PRI_LOW], OMCI_MULGET_STATE_RUN,  \
                        OMCI_MULGET_EVT_GET,    (OMCI_FSMEVTHDL)omci_MulGetGetOnRun);

    gOmciMulGetFsm[OMCI_MSG_BASELINE_PRI_HIGH] = omci_FsmCreate(OMCI_MULGET_STATE_IDLE, OMCI_MULGET_STATE_NUM, OMCI_MULGET_EVT_NUM);
    omci_AddFsmEventHandler(gOmciMulGetFsm[OMCI_MSG_BASELINE_PRI_HIGH], OMCI_MULGET_STATE_IDLE, \
                        OMCI_MULGET_EVT_START,  (OMCI_FSMEVTHDL)omci_MulGetStartOnIdle);
    omci_AddFsmEventHandler(gOmciMulGetFsm[OMCI_MSG_BASELINE_PRI_HIGH], OMCI_MULGET_STATE_IDLE, \
                        OMCI_MULGET_EVT_GET,    (OMCI_FSMEVTHDL)omci_MulGetGetOnIdle);
    omci_AddFsmEventHandler(gOmciMulGetFsm[OMCI_MSG_BASELINE_PRI_HIGH], OMCI_MULGET_STATE_RUN,  \
                        OMCI_MULGET_EVT_START,  (OMCI_FSMEVTHDL)omci_MulGetStartOnRun);
    omci_AddFsmEventHandler(gOmciMulGetFsm[OMCI_MSG_BASELINE_PRI_HIGH], OMCI_MULGET_STATE_RUN,  \
                        OMCI_MULGET_EVT_GET,    (OMCI_FSMEVTHDL)omci_MulGetGetOnRun);

    /* Mib Upload state machine */
    memset(&gOmciMibUploadData, 0x00, sizeof(omci_mib_upload_info_ts));

    gOmciMibUploadFsm = omci_FsmCreate(OMCI_MIB_UPLOAD_STATE_IDLE, OMCI_MIB_UPLOAD_STATE_NUM, OMCI_MIB_UPLOAD_EVT_NUM);
    omci_AddFsmEventHandler(gOmciMibUploadFsm, OMCI_MIB_UPLOAD_STATE_IDLE, \
                        OMCI_MIB_UPLOAD_EVT_START,  (OMCI_FSMEVTHDL)omci_MibUploadStartOnIdle);
    omci_AddFsmEventHandler(gOmciMibUploadFsm, OMCI_MIB_UPLOAD_STATE_IDLE, \
                        OMCI_MIB_UPLOAD_EVT_GET,    (OMCI_FSMEVTHDL)omci_MibUploadGetOnIdle);
    omci_AddFsmEventHandler(gOmciMibUploadFsm, OMCI_MIB_UPLOAD_STATE_RUN,  \
                        OMCI_MIB_UPLOAD_EVT_START,  (OMCI_FSMEVTHDL)omci_MibUploadStartOnRun);
    omci_AddFsmEventHandler(gOmciMibUploadFsm, OMCI_MIB_UPLOAD_STATE_RUN,  \
                        OMCI_MIB_UPLOAD_EVT_GET,    (OMCI_FSMEVTHDL)omci_MibUploadGetOnRun);

    return GOS_OK;
}


GOS_ERROR_CODE OMCI_OnCreateMsg(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    void*                   pMibRowBuff = omci_GetMibRowBuff();
    omci_me_class_t         classID     = (omci_me_class_t)pNorOmciMsg->meId.meClass;
    omci_me_instance_t      entityID    = (omci_me_instance_t)pNorOmciMsg->meId.meInstance;
    MIB_TABLE_INDEX         tableIndex  = MIB_GetTableIndexByClassId(classID);
    UINT32                  entrySize   = MIB_GetTableEntrySize(tableIndex);
    omci_msg_attr_mask_t    attrsSet    = omci_GetOltAccAttrSet(tableIndex, OMCI_ME_ATTR_ACCESS_SBC);
    BOOL                    meValid;
    BOOL                    meExist;
    MIB_ATTRS_SET           validMibSet;


    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Handling CREATE msg: class=%d, entity=0x%x",
                (omci_me_class_t)pNorOmciMsg->meId.meClass, (omci_me_instance_t)pNorOmciMsg->meId.meInstance);

    if (GOS_OK != OMCI_CheckIsMeValid(pNorOmciMsg->meId.meClass, &pNorOmciMsg->meId.meInstance, &meValid, &meExist))
    {
        return GOS_FAIL;
    }

    if (meValid == FALSE || (meExist == TRUE && FAL_OK != feature_api(FEATURE_API_ME_00000004)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,"OnCreateMsg: unknown entity or entity exists in MIB, class=%d, entity=0x%x",
                    (omci_me_class_t)pNorOmciMsg->meId.meClass, (omci_me_instance_t)pNorOmciMsg->meId.meInstance);
        OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_INSTANCE_EXISTS, TRUE);

        return GOS_FAIL;
    }

    if (TRUE == meExist)
    {
        MIB_SetAttrToBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &entityID, pMibRowBuff, sizeof(omci_me_instance_t));

        if (GOS_OK != MIB_Get(tableIndex, pMibRowBuff, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_WARN,"OnCreateMsg: entity not exist in MIB, class=%d, entity=0x%x", classID, entityID);
            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_UNKNOWN_ME_INSTANCE, TRUE);
        }
    }
    else
    {
        MIB_Default(tableIndex, pMibRowBuff, entrySize);
        MIB_SetAttrToBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &entityID, pMibRowBuff, sizeof(omci_me_instance_t));
    }
    omci_OmciDataToMibRow(tableIndex, &(pNorOmciMsg->content[0]), pMibRowBuff, attrsSet);

    omci_AttrsOmciSetToMibSet(&attrsSet, &validMibSet);
    if (TRUE == meExist)
    {
        // Save in MIB
        if (GOS_OK != MIB_SetAttributes(tableIndex, pMibRowBuff, entrySize, &validMibSet))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_WARN,"OnCreateMsg: MIB_Set error, class=%d, entity=0x%x, attrSet=0x%x",
                      classID, entityID, validMibSet);
            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
        }
    }
    else
    {
        // Create ME in MIB
        if (GOS_OK != MIB_Set(tableIndex, pMibRowBuff, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_WARN,"OnCreateMsg: MIB_Set error, class=%d, entity=0x%x", classID, entityID);
            OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);

            return GOS_FAIL;
        }
    }
    if (GOS_OK != OMCI_MeOperCfg(tableIndex, pMibRowBuff, pMibRowBuff, (FALSE == meExist) ? MIB_ADD : MIB_SET, validMibSet, pNorOmciMsg->priority))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,"OnCreateMsg: OMCI_MeOperCfg error, class=%d, entity=0x%x", classID, entityID);
        MIB_Delete(tableIndex, pMibRowBuff, entrySize);
        OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);

        return GOS_FAIL;
    }

    omci_SyncMibData();
    OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSED_SUCCESSFULLY, TRUE);


    // Create Related ME
    if (GOS_OK != omci_CreateRelatedMe(tableIndex, pMibRowBuff))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,"OnCreateMsg: CreateRelatedMe error, class=%d, entity=0x%x", classID, entityID);
    }

    return GOS_OK;
}


GOS_ERROR_CODE OMCI_CheckIsMeValid(UINT16 MeClassID, UINT16* pMeInstance, BOOL* pMeValid, BOOL* pMeExist)
{
    CHAR* pMibRowBuff = omci_GetMibRowBuff();
    MIB_TABLE_INDEX tableIndex;


    GOS_ASSERT(pMeInstance != NULL);
    GOS_ASSERT(pMeValid != NULL);
    GOS_ASSERT(pMeExist != NULL);

    *pMeValid = FALSE;
    *pMeExist = FALSE;

    tableIndex = MIB_GetTableIndexByClassId((omci_me_class_t)MeClassID);

    if (tableIndex != MIB_TABLE_UNKNOWN_INDEX)
    {
        *pMeValid = TRUE;

        MIB_SetAttrToBuf(tableIndex, MIB_ATTR_FIRST_INDEX, pMeInstance, pMibRowBuff, sizeof(omci_me_instance_t));

        if (GOS_OK == MIB_Get(tableIndex, pMibRowBuff, MIB_GetTableEntrySize(tableIndex)))
        {
            *pMeExist = TRUE;
        }
    }

    return GOS_OK;
}




GOS_ERROR_CODE OMCI_AttrsMibSetToOmciSet(MIB_ATTRS_SET* pMibAttrSet, omci_msg_attr_mask_t* pOmciAttrSet)
{
    MIB_ATTR_INDEX attrIndex;
    UINT32 i;

    *pOmciAttrSet = 0;

    for (attrIndex = MIB_ATTR_NEXT_INDEX(MIB_ATTR_FIRST_INDEX), i = 0; i < sizeof(omci_msg_attr_mask_t) * 8;
         i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        if (MIB_IsInAttrSet(pMibAttrSet, attrIndex))
        {
            *pOmciAttrSet = (*pOmciAttrSet) | (0x8000 >> i);
        }
    }

    return GOS_OK;
}


GOS_ERROR_CODE OMCI_OnDeleteMsg(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    MIB_TABLE_INDEX tableIndex;
    UINT32          entrySize;
    void*           pMibRowBuff;
    BOOL            meValid;
    BOOL            meExist;

    OMCI_LOG(OMCI_LOG_LEVEL_WARN,"Handling DELETE msg: class=%d, entity=0x%x",
                (omci_me_class_t)pNorOmciMsg->meId.meClass, (omci_me_instance_t)pNorOmciMsg->meId.meInstance);

    if (GOS_OK != OMCI_CheckIsMeValid(pNorOmciMsg->meId.meClass, &pNorOmciMsg->meId.meInstance, &meValid, &meExist))
    {
        return GOS_FAIL;
    }

    if (meValid == FALSE || meExist == FALSE)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,"OnDeleteMsg: entity not exist in MIB, class=%d, entity=0x%x",
                    (omci_me_class_t)pNorOmciMsg->meId.meClass, (omci_me_instance_t)pNorOmciMsg->meId.meInstance);
        OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_UNKNOWN_ME_INSTANCE, TRUE);

        return GOS_FAIL;
    }

    tableIndex  = MIB_GetTableIndexByClassId((omci_me_class_t)pNorOmciMsg->meId.meClass);
    entrySize   = MIB_GetTableEntrySize(tableIndex);
    pMibRowBuff = omci_GetMibRowBuff();

    MIB_SetAttrToBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &pNorOmciMsg->meId.meInstance, pMibRowBuff, sizeof(omci_me_instance_t));

    if(MIB_Get(tableIndex, pMibRowBuff, entrySize)!=GOS_OK)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,"OnDeleteMsg: can't find old value");
        return GOS_FAIL;
    }

    OMCI_MeOperCfg(tableIndex, pMibRowBuff, pMibRowBuff, MIB_DEL, 0, pNorOmciMsg->priority);
    // Delete ME in MIB
    if (GOS_OK != MIB_Delete(tableIndex, pMibRowBuff, entrySize))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,"OnDeleteMsg: MIB_Delete error, class=%d, entity=0x%x",
                    (omci_me_class_t)pNorOmciMsg->meId.meClass, (omci_me_instance_t)pNorOmciMsg->meId.meInstance);
        OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);

        return GOS_FAIL;
    }

    omci_SyncMibData();
    OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSED_SUCCESSFULLY, TRUE);

    return GOS_OK;
}


GOS_ERROR_CODE OMCI_OnSetMsg(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    MIB_TABLE_INDEX         tableIndex;
    omci_msg_attr_mask_t    attrOmciSet;
    MIB_ATTRS_SET           validMibSet;
    UINT32                  attrSize;
    MIB_ATTR_INDEX          attrIndex;
    BOOL                    meValid;
    BOOL                    meExist;
    UINT32                  i;

    OMCI_LOG(OMCI_LOG_LEVEL_WARN,"Handling SET msg: class=%d, entity=0x%x",
                (omci_me_class_t)pNorOmciMsg->meId.meClass, (omci_me_instance_t)pNorOmciMsg->meId.meInstance);

    if (GOS_OK != OMCI_CheckIsMeValid(pNorOmciMsg->meId.meClass, &pNorOmciMsg->meId.meInstance, &meValid, &meExist))
    {
        return GOS_FAIL;
    }

    if (meValid == FALSE || meExist == FALSE)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,"OnSetMsg: entity not exist in MIB, class=%d, entity=0x%x",
                    (omci_me_class_t)pNorOmciMsg->meId.meClass, (omci_me_instance_t)pNorOmciMsg->meId.meInstance);
        OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_UNKNOWN_ME_INSTANCE, TRUE);

        return GOS_FAIL;
    }

    tableIndex  = MIB_GetTableIndexByClassId((omci_me_class_t)pNorOmciMsg->meId.meClass);
    attrOmciSet = (omci_msg_attr_mask_t)GOS_Ntohs(GOS_GetUINT16((UINT16*)&pNorOmciMsg->content[0]));

    omci_AttrsOmciSetToMibSet(&attrOmciSet, &validMibSet);

    // Get total attribute size in set
    for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0, attrSize = 0; i < MIB_GetTableAttrNum(tableIndex);
         i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        if (MIB_IsInAttrSet(&validMibSet, attrIndex))
        {
            attrSize += MIB_GetAttrLen(tableIndex, attrIndex);
        }
    }

    return omci_SingleSet(pNorOmciMsg);
}


GOS_ERROR_CODE OMCI_OnGetMsg(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    MIB_TABLE_INDEX tableIndex;
    MIB_ATTRS_SET   validMibSet;
    UINT32          attrSize;
    MIB_ATTR_INDEX  attrIndex;
    BOOL            meValid;
    BOOL            meExist;
    UINT32          i;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Handling GET msg: class=%d, entity=0x%x",
                (omci_me_class_t)pNorOmciMsg->meId.meClass, (omci_me_instance_t)pNorOmciMsg->meId.meInstance);

    if (GOS_OK != OMCI_CheckIsMeValid(pNorOmciMsg->meId.meClass, &pNorOmciMsg->meId.meInstance, &meValid, &meExist))
    {
        return GOS_FAIL;
    }

    if (meValid == FALSE || meExist == FALSE)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,"OnGetMsg: entity not exist in MIB, class=%d, entity=0x%x",
                    (omci_me_class_t)pNorOmciMsg->meId.meClass, (omci_me_instance_t)pNorOmciMsg->meId.meInstance);
        OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_UNKNOWN_ME_INSTANCE, TRUE);

        return GOS_FAIL;
    }

    tableIndex = MIB_GetTableIndexByClassId((omci_me_class_t)pNorOmciMsg->meId.meClass);


    omci_msg_attr_mask_t    supportOmciSet = omci_GetOptionAttrSet(tableIndex, OMCI_ME_ATTR_TYPE_OPTIONAL) | omci_GetOptionAttrSet(tableIndex, OMCI_ME_ATTR_TYPE_MANDATORY);
    omci_msg_attr_mask_t    attrOmciSet, validOmciSet;
    attrOmciSet = (omci_msg_attr_mask_t)GOS_Ntohs(GOS_GetUINT16((UINT16*)&pNorOmciMsg->content[0]));
    // validSet indicats the valid attributes that can be get
    validOmciSet = attrOmciSet & supportOmciSet;
    omci_AttrsOmciSetToMibSet(&validOmciSet, &validMibSet);


    // To check if it is a multi-get
    for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0, attrSize = 0; i < MIB_GetTableAttrNum(tableIndex);
         i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        if (MIB_IsInAttrSet(&validMibSet, attrIndex))
        {
            if (MIB_ATTR_TYPE_TABLE == MIB_GetAttrDataType(tableIndex, attrIndex))
            {
                return omci_MultiGet(pNorOmciMsg);
            }
            else
            {
                attrSize = MIB_GetAttrLen(tableIndex, attrIndex);
                if (attrSize > OMCI_MSG_BASELINE_GET_RSP_LIMIT)
                {
                    return omci_MultiGet(pNorOmciMsg);
                }
            }
        }
    }

    return omci_SingleGet(pNorOmciMsg);
}


GOS_ERROR_CODE OMCI_OnGetNextMsg(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    UINT32 pri = pNorOmciMsg->priority;

    GOS_ASSERT(pri < OMCI_MSG_BASELINE_PRI_NUM);

    omci_FsmRun(gOmciMulGetFsm[pri], OMCI_MULGET_EVT_GET, pNorOmciMsg);

    return GOS_OK;
}


GOS_ERROR_CODE OMCI_OnMibReset(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    OMCI_LOG(OMCI_LOG_LEVEL_INFO,"OLT reset MIB");

#if !defined(FPGA_DEFINED) /* in FPGA, the reset is too long to OMCI timeout */
    omci_wrapper_resetMib();
#endif
#if 0
    if (GOS_OK == OMCI_ResetMib())
    {
        OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSED_SUCCESSFULLY, TRUE);
    }
    else
    {
        OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
    }
#else
    OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSED_SUCCESSFULLY, TRUE);
    OMCI_ResetMib();
#endif
    return GOS_OK;
}


static UINT16   gAlmSnapShotCnt;

GOS_ERROR_CODE OMCI_OnGetAllAlarms(omci_msg_norm_baseline_t     *pNorOmciMsg)
{
    GOS_ERROR_CODE      ret = GOS_OK;
    UINT8               almRetrievalMode;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Handling Action: Get All Alarms");

    // create timer for snapshot control
    ret = omci_timer_create(OMCI_TIMER_RESERVED_CLASS_ID,
                            OMCI_TIMER_RESERVED_ALM_INSTANCE_ID,
                            OMCI_TIMER_ALM_UPLOAD_INTERVAL_SECS,
                            0, FALSE, 0, omci_alarm_snapshot_timer_handler,OMCI_TIMER_SIG_GET_ALM);
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Create timer for get alarms fail");

        OMCI_ResponseMsg(pNorOmciMsg, 0, TRUE);

        return GOS_FAIL;
    }

    // reset alarm seq num
    omci_alarm_reset_sequence_number();

    // retrieve alarm retrieval mode
    almRetrievalMode = pNorOmciMsg->content[0];

    // generate snapshot
    gAlmSnapShotCnt = omci_alarm_snapshot_create_all(almRetrievalMode);

    // delete timer for empty snapshot
    if (0 == gAlmSnapShotCnt)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "There is no alarm snapshot exist");

        // delete timer
        omci_timer_delete_by_id(OMCI_TIMER_RESERVED_CLASS_ID,
                                OMCI_TIMER_RESERVED_ALM_INSTANCE_ID);
    }

    // copy data
    GOS_SetUINT16((UINT16 *)&pNorOmciMsg->content, GOS_Htons(gAlmSnapShotCnt));

    // send response
    OMCI_ResponseMsg(pNorOmciMsg, pNorOmciMsg->content[0], FALSE);

    return GOS_OK;
}

GOS_ERROR_CODE OMCI_OnGetAllAlarmsNext(omci_msg_norm_baseline_t     *pNorOmciMsg)
{
    GOS_ERROR_CODE      ret = GOS_OK;
    UINT16              almSnapShotSeq;
    omci_me_class_t     classID;
    omci_me_instance_t  instanceID;
    mib_alarm_table_t   alarmTable;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Handling Action: Get All Alarms Next");

    // retrieve alarm sequence number
    almSnapShotSeq = GOS_Ntohs(GOS_GetUINT16((UINT16 *)&pNorOmciMsg->content));

    // retrieve instanceID & alarm table data
    ret = omci_alarm_snapshot_get_by_seq(almSnapShotSeq,
                                        &classID,
                                        &instanceID,
                                        &alarmTable);
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Get all alarms next fail");

        OMCI_ResponseMsg(pNorOmciMsg, 0, TRUE);

        return GOS_FAIL;
    }

    // restart timer
    if (++almSnapShotSeq >= gAlmSnapShotCnt)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Getting last alarm, destroy timer");

        // delete all snapshot
        omci_alarm_snapshot_delete_all();

        // delete timer
        omci_timer_delete_by_id(OMCI_TIMER_RESERVED_CLASS_ID,
                                OMCI_TIMER_RESERVED_ALM_INSTANCE_ID);
    }
    else
    {
        ret = omci_timer_restart(OMCI_TIMER_RESERVED_CLASS_ID,
                                OMCI_TIMER_RESERVED_ALM_INSTANCE_ID,
                                OMCI_TIMER_ALM_UPLOAD_INTERVAL_SECS,
                                0, FALSE);
        if (GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Restart timer for get alarms next fail");

            OMCI_ResponseMsg(pNorOmciMsg, 0, TRUE);

            return GOS_FAIL;
        }
    }

    // copy data
    GOS_SetUINT16((UINT16 *)&pNorOmciMsg->content[0], GOS_Htons(classID));
    GOS_SetUINT16((UINT16 *)&pNorOmciMsg->content[2], GOS_Htons(instanceID));
    memcpy(&pNorOmciMsg->content[4], &alarmTable, sizeof(mib_alarm_table_t));

    // send response
    OMCI_ResponseMsg(pNorOmciMsg, pNorOmciMsg->content[0], FALSE);

    return GOS_OK;
}

GOS_ERROR_CODE OMCI_OnTest(omci_msg_norm_baseline_t     *pNorOmciMsg)
{
    MIB_TABLE_INDEX             tableIndex;
    MIB_TABLE_T                 *pTable;
    omci_msg_norm_baseline_t    *pForkNorOmciMsg;
    CHAR                        taskName[OMCI_TASK_NAME_MAX_LEN];
    OMCI_TASK_ID                taskID;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Handling Action: Test");

    tableIndex = MIB_GetTableIndexByClassId(pNorOmciMsg->meId.meClass);

    // break for undefined ME handler
    pTable = mib_GetTablePtr(tableIndex);
    if (!pTable || !pTable->meOper || !pTable->meOper->meOperTestHandler)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "No test handler has defined: %s",
            MIB_GetTableName(tableIndex));

        // send not supported response
        return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_CMD_NOT_SUPPORTED, TRUE);
    }

    // copy omci message in order to pass it further
    pForkNorOmciMsg = malloc(sizeof(omci_msg_norm_baseline_t));
    if (!pForkNorOmciMsg)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Allocate memory for test handler fail: %s",
            MIB_GetTableName(tableIndex));

        // send process error response
        return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
    }
    memcpy(pForkNorOmciMsg, pNorOmciMsg, sizeof(omci_msg_norm_baseline_t));

    snprintf(taskName, OMCI_TASK_NAME_MAX_LEN, "test on %d/%#x",
        pNorOmciMsg->meId.meClass, pNorOmciMsg->meId.meInstance);

    // create task for the test
    taskID = OMCI_SpawnTask(taskName,
                            pTable->meOper->meOperTestHandler,
                            pForkNorOmciMsg,
                            OMCI_TASK_PRI_TEST,
                            FALSE);
    if (OMCI_TASK_ID_INVALID == taskID)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Fork task for test handler fail: %s",
            MIB_GetTableName(tableIndex));

        // send process error response
        return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
    }

    // send successful response
    return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSED_SUCCESSFULLY, TRUE);
}

GOS_ERROR_CODE OMCI_OnStartSWDL(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    omci_me_class_t     classID     = (omci_me_class_t)pNorOmciMsg->meId.meClass;
    omci_me_instance_t  instanceID  = (omci_me_instance_t)pNorOmciMsg->meId.meInstance;

    MIB_TABLE_SWIMAGE_T mibSoftwareImage;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Handling Action: Start Software Download %x\n", instanceID);

    if (OMCI_ME_CLASS_SOFTWARE_IMAGE == classID)
    {
        // check if instance exists or not by using instanceID
        mibSoftwareImage.EntityID = instanceID;
        if (GOS_OK != MIB_Get(MIB_TABLE_SWIMAGE_INDEX, &mibSoftwareImage, sizeof(MIB_TABLE_SWIMAGE_T)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "InstanceID not exist in MIB, class=%d, instance=0x%x", classID, instanceID);

            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_UNKNOWN_ME_INSTANCE, TRUE);
        }

        // return failure if any one of is_committed or is_active flag is true
        if (mibSoftwareImage.Committed || mibSoftwareImage.Active)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Software Download for instance being committed or active is prohibited");

            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PARAMETER_ERROR, TRUE);
        }
    }
    else
    {
        if (FAL_OK != feature_api(FEATURE_API_ME_00000020_IS_SUPPORTED, &classID))
            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_CMD_NOT_SUPPORTED, TRUE);
    }

    // wordaround for too much section at once will make NIC out of buffer
    if ((pNorOmciMsg->content[0]) > OMCI_SW_DL_MAX_ACCEPTABLE_WINDOW_SIZE)
        gOmciSwDlCtrl.windowSize = OMCI_SW_DL_MAX_ACCEPTABLE_WINDOW_SIZE;
    // retrieve window size and image byte size
    else
        gOmciSwDlCtrl.windowSize = pNorOmciMsg->content[0];

    gOmciSwDlCtrl.imageByteSize = (pNorOmciMsg->content[1] << 24) + (pNorOmciMsg->content[2] << 16) +
        (pNorOmciMsg->content[3] << 8) + pNorOmciMsg->content[4];

    // remove the image file
    if(remove(OMCI_SW_DL_IMAGE_LOCATION)!=0){
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Delete image failed...\n");
        }

    // reset the download section flag
    memset(gOmciSwDlCtrl.dlSectionFlag, 0, OMCI_SW_DL_SECTION_BITMASK_IN_BYTE);

    // release the download window buffer if necessary
    if (gOmciSwDlCtrl.pDlWindowBuffer)
    {
        free(gOmciSwDlCtrl.pDlWindowBuffer);
        gOmciSwDlCtrl.pDlWindowBuffer = NULL;
    }

    // for OMCI baseline message set, every section is fixed to 31 bytes
    gOmciSwDlCtrl.pDlWindowBuffer =
        malloc(OMCI_SW_DL_MAX_BYTE_OF_SECTION_IN_BASELINE_MSG * (gOmciSwDlCtrl.windowSize + 1));
    if (!gOmciSwDlCtrl.pDlWindowBuffer)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "There is no memory available for Software Download handling");

        return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
    }

    if (FAL_OK != feature_api(FEATURE_API_ME_00000002) && OMCI_ME_CLASS_SOFTWARE_IMAGE == classID)
    {
        char    flagName[OMCI_SW_DL_IMAGE_UBOOT_FLAG_NAME];
        char    flagBuffer[OMCI_SW_DL_IMAGE_UBOOT_FLAG_BUFFER];

        // before flashing, mark the partition as invalid
        mibSoftwareImage.Valid = FALSE;
        if (GOS_OK != MIB_Set(MIB_TABLE_SWIMAGE_INDEX, &mibSoftwareImage, sizeof(MIB_TABLE_SWIMAGE_T)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Modify image valid state from TRUE to FALSE fail");

            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
        }

        // rewrite the sw_validX according to the real instanceID
        sprintf(flagName, "sw_valid%u", mibSoftwareImage.EntityID);
        sprintf(flagBuffer, "%u", mibSoftwareImage.Valid);
        if (omci_sw_dl_uboot_env_set(flagName, flagBuffer))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Modify image valid state from TRUE to FALSE fail");

            // rollback the image valid state
            mibSoftwareImage.Valid = TRUE;
            MIB_Set(MIB_TABLE_SWIMAGE_INDEX, &mibSoftwareImage, sizeof(MIB_TABLE_SWIMAGE_T));

            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
        }
    }

    // reset the image state
    gOmciSwDlCtrl.bCrcCheckDone = FALSE;
    gOmciSwDlCtrl.bFlashingState = FALSE;

    // sync mib
    omci_SyncMibData();

    // prepare response message content
    memset(pNorOmciMsg->content, 0x00, OMCI_MSG_BASELINE_CONTENTS_LEN);
    pNorOmciMsg->content[1] = gOmciSwDlCtrl.windowSize;

    // send out the response
    OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSED_SUCCESSFULLY, FALSE);

    return GOS_OK;
}

GOS_ERROR_CODE OMCI_OnDLSection(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    omci_me_class_t     classID     = (omci_me_class_t)pNorOmciMsg->meId.meClass;
    omci_me_instance_t  instanceID  = (omci_me_instance_t)pNorOmciMsg->meId.meInstance;

    MIB_TABLE_SWIMAGE_T mibSoftwareImage;
    UINT8               dlSectionNumber;

    if (OMCI_ME_CLASS_SOFTWARE_IMAGE == classID)
    {
        // check if instance exists or not by using instanceID
        mibSoftwareImage.EntityID = instanceID;
        if (GOS_OK != MIB_Get(MIB_TABLE_SWIMAGE_INDEX, &mibSoftwareImage, sizeof(MIB_TABLE_SWIMAGE_T)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "InstanceID not exist in MIB, class=%d, instance=0x%x", classID, instanceID);

            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_UNKNOWN_ME_INSTANCE, TRUE);;
        }

        // return failure if any one of is_committed or is_active flag is true
        if (mibSoftwareImage.Committed || mibSoftwareImage.Active)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Software Download for instance being committed or active is prohibited");

            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PARAMETER_ERROR, TRUE);
        }
    }
    else
    {
        if (FAL_OK != feature_api(FEATURE_API_ME_00000020_IS_SUPPORTED, &classID))
            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_CMD_NOT_SUPPORTED, TRUE);
    }

    // reject download section if the window buffer is never created before
    if (!gOmciSwDlCtrl.pDlWindowBuffer)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "The window buffer memory is never created before");

        return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
    }

    // retrieve download section number
    dlSectionNumber = pNorOmciMsg->content[0];

    // mark the download section as recevied
    gOmciSwDlCtrl.dlSectionFlag[dlSectionNumber / 8] |= (0x80 >> (dlSectionNumber % 8));

    // store the section into the window buffer
    memcpy(gOmciSwDlCtrl.pDlWindowBuffer + OMCI_SW_DL_MAX_BYTE_OF_SECTION_IN_BASELINE_MSG * dlSectionNumber,
        &pNorOmciMsg->content[1], OMCI_SW_DL_MAX_BYTE_OF_SECTION_IN_BASELINE_MSG);

    // if AR bit is set, check if it is the last section of a window
    if (1 == pNorOmciMsg->ar)
    {
        FILE    *pFD;
        UINT32  currImageSize, i, j;
        UINT16  windowSizeInBytes;

        windowSizeInBytes = OMCI_SW_DL_MAX_BYTE_OF_SECTION_IN_BASELINE_MSG * (gOmciSwDlCtrl.windowSize + 1);

        // check if we recevied all the sections
        for (i = 0; i < (dlSectionNumber / 8); i++)
        {
            if (gOmciSwDlCtrl.dlSectionFlag[i] != 0xFF)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "At least one download section is missing");

                // reset the download section flag
                memset(gOmciSwDlCtrl.dlSectionFlag, 0, OMCI_SW_DL_SECTION_BITMASK_IN_BYTE);

                return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
            }
        }
        for (j = 0; j <= (dlSectionNumber % 8); j++)
        {
            if (!(gOmciSwDlCtrl.dlSectionFlag[i] & (0x80 >> j)))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "At least one download section is missing");

                // reset the download section flag
                memset(gOmciSwDlCtrl.dlSectionFlag, 0, OMCI_SW_DL_SECTION_BITMASK_IN_BYTE);

                return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
            }
        }

        // check if the image file buffer is available or not
        pFD = fopen(OMCI_SW_DL_IMAGE_LOCATION, "ab");
        if (!pFD)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "The image file buffer cannot be opened for appending");

            // reset the download section flag
            memset(gOmciSwDlCtrl.dlSectionFlag, 0, OMCI_SW_DL_SECTION_BITMASK_IN_BYTE);

            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
        }

        fseek(pFD, 0, SEEK_END);
        currImageSize = ftell(pFD);

        // if it is the last window, the padding part (if exist) needs to be skipped
        if ((gOmciSwDlCtrl.imageByteSize - currImageSize) < windowSizeInBytes)
        {
            for (i = 0; i < (gOmciSwDlCtrl.imageByteSize - currImageSize); i++)
                fprintf(pFD, "%c", gOmciSwDlCtrl.pDlWindowBuffer[i]);
        }
        // if it is not the last window, just append the window into the image file buffer
        else
        {
            for (i = 0; i < windowSizeInBytes; i++)
                fprintf(pFD, "%c", gOmciSwDlCtrl.pDlWindowBuffer[i]);
        }

        fclose(pFD);

        // reset the download section flag
        memset(gOmciSwDlCtrl.dlSectionFlag, 0, OMCI_SW_DL_SECTION_BITMASK_IN_BYTE);

        // prepare response message content
        memset(pNorOmciMsg->content, 0x00, OMCI_MSG_BASELINE_CONTENTS_LEN);
        pNorOmciMsg->content[1] = dlSectionNumber;

        // send out the response
        OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSED_SUCCESSFULLY, FALSE);
        OMCI_ResetHistoryRspMsg ();
        OMCI_ResetLastRspMsgIndex ();
    }

    return GOS_OK;
}

int omci_sw_dl_uboot_env_set(char *name, char *buffer)
{
    char                flagName[OMCI_SW_DL_IMAGE_UBOOT_FLAG_NAME];
    char                flagBuffer[OMCI_SW_DL_IMAGE_UBOOT_FLAG_BUFFER];
    char                execCommand[OMCI_SW_DL_IMAGE_UBOOT_FLAG_COMMAND_BUFFER];
    int                 execCmdStatus;

    if (!name || !buffer)
        return -1;

    memset(flagName, 0, OMCI_SW_DL_IMAGE_UBOOT_FLAG_NAME);
    memset(flagBuffer, 0, OMCI_SW_DL_IMAGE_UBOOT_FLAG_BUFFER);
    memset(execCommand, 0, OMCI_SW_DL_IMAGE_UBOOT_FLAG_COMMAND_BUFFER);

    snprintf(flagName, OMCI_SW_DL_IMAGE_UBOOT_FLAG_NAME, "%s", name);
    snprintf(flagBuffer, OMCI_SW_DL_IMAGE_UBOOT_FLAG_BUFFER, "%s", buffer);
#ifdef OMCI_X86
    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "X86 not support /bin/nv setenv %s %s", flagName, flagBuffer);
#else
    sprintf(execCommand, "/bin/nv setenv %s %s", flagName, flagBuffer);

    execCmdStatus = system(execCommand);
    if (-1 == execCmdStatus || FALSE == WIFEXITED(execCmdStatus) || 0 != WEXITSTATUS(execCmdStatus))
    {
        return -1;
    }
#endif
    return 0;
}

char* omci_sw_dl_uboot_env_get(char *name)
{
    char                flagName[OMCI_SW_DL_IMAGE_UBOOT_FLAG_NAME];
    char                flagBuffer[OMCI_SW_DL_IMAGE_UBOOT_FLAG_BUFFER];
    char                execCommand[OMCI_SW_DL_IMAGE_UBOOT_FLAG_COMMAND_BUFFER];
    char                *pStr, *pReturnStr = NULL;
    FILE                *pFD;

    if (!name)
        return pReturnStr;

    memset(flagName, 0, OMCI_SW_DL_IMAGE_UBOOT_FLAG_NAME);
    memset(flagBuffer, 0, OMCI_SW_DL_IMAGE_UBOOT_FLAG_BUFFER);
    memset(execCommand, 0, OMCI_SW_DL_IMAGE_UBOOT_FLAG_COMMAND_BUFFER);

    snprintf(flagName, OMCI_SW_DL_IMAGE_UBOOT_FLAG_NAME, "%s", name);
#ifdef OMCI_X86
    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "X86 not support /bin/nv getenv %s", flagName);
#else
    sprintf(execCommand, "/bin/nv getenv %s", flagName);

    pFD = popen(execCommand, "r");
    if (pFD)
    {
        if (fgets(flagBuffer, OMCI_SW_DL_IMAGE_UBOOT_FLAG_BUFFER, pFD))
        {
            strtok(flagBuffer, "=");

            if ((pStr = strtok(NULL, "")))
            {
                pReturnStr = strdup(pStr);
                pReturnStr[strlen(pReturnStr) - 1] = '\0';
            }
        }

        pclose(pFD);
    }
#endif
    return pReturnStr;
}

void omci_sw_dl_flashing_state_updater(BOOL bWrite, BOOL *bFlashingState)
{
    pthread_mutex_lock(&gOmciSwDlFlashingStateMutex);

    if (bWrite)
        gOmciSwDlCtrl.bFlashingState = *bFlashingState;
    else
        *bFlashingState = gOmciSwDlCtrl.bFlashingState;

    pthread_mutex_unlock(&gOmciSwDlFlashingStateMutex);
}

void* omci_sw_dl_upgrade_procedure_thread(void *arg)
{
    omci_msg_meid_t     *pMeId;
    omci_me_class_t     meClass;
    omci_me_instance_t  meInstance;
    MIB_TABLE_SWIMAGE_T mibSoftwareImage;
    BOOL                bFlashingState;
    BOOL                *pbFlashingRet = malloc(sizeof(BOOL));
    char                flagName[OMCI_SW_DL_IMAGE_UBOOT_FLAG_NAME];
    char                flagBuffer[OMCI_SW_DL_IMAGE_UBOOT_FLAG_BUFFER];
    char                *pStr;
    char                execCommand[OMCI_SW_DL_IMAGE_UBOOT_FLAG_COMMAND_BUFFER];
    int                 execCmdStatus;

    pMeId = (omci_msg_meid_t *)arg;
    meClass = pMeId->meClass;
    meInstance = pMeId->meInstance;

    if (OMCI_ME_CLASS_SOFTWARE_IMAGE == meClass)
    {
        // retrieve the instance data by using instanceID
        mibSoftwareImage.EntityID = meInstance;
        if (GOS_OK != MIB_Get(MIB_TABLE_SWIMAGE_INDEX, &mibSoftwareImage, sizeof(MIB_TABLE_SWIMAGE_T)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_SWIMAGE_INDEX), meInstance);
        }
        if (FAL_OK == feature_api(FEATURE_API_ME_00000002))
        {
            // before flashing, mark the partition as invalid
            mibSoftwareImage.Valid = FALSE;
            if (GOS_OK != MIB_Set(MIB_TABLE_SWIMAGE_INDEX, &mibSoftwareImage, sizeof(MIB_TABLE_SWIMAGE_T)))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Modify image valid state from TRUE to FALSE fail");

                *pbFlashingRet = FALSE;

                goto out;
            }

            // rewrite the sw_validX according to the real instanceID
            sprintf(flagName, "sw_valid%u", mibSoftwareImage.EntityID);
            sprintf(flagBuffer, "%u", mibSoftwareImage.Valid);
            if (omci_sw_dl_uboot_env_set(flagName, flagBuffer))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Modify image valid state from TRUE to FALSE fail");

                // rollback the image valid state
                mibSoftwareImage.Valid = TRUE;
                MIB_Set(MIB_TABLE_SWIMAGE_INDEX, &mibSoftwareImage, sizeof(MIB_TABLE_SWIMAGE_T));

                *pbFlashingRet = FALSE;

                goto out;
            }
        }
    }

    if (OMCI_ME_CLASS_SOFTWARE_IMAGE == meClass)
    {
        // drop cache for release memory space
        execCmdStatus = system("echo 1 > /proc/sys/vm/drop_caches");
        if (-1 == execCmdStatus || FALSE == WIFEXITED(execCmdStatus) || 0 != WEXITSTATUS(execCmdStatus))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "drop cache fail");

            *pbFlashingRet = FALSE;

            goto out;
        }

        // start to flashing the image
        sprintf(execCommand,
            "/bin/sh -x /etc/scripts/fwu_starter.sh %d " OMCI_SW_DL_IMAGE_LOCATION, meInstance);
        execCmdStatus = system(execCommand);
        if (-1 == execCmdStatus || FALSE == WIFEXITED(execCmdStatus) || 0 != WEXITSTATUS(execCmdStatus))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Execute image flashing script fail");

            *pbFlashingRet = FALSE;

            goto out;
        }
    }
    else
    {
        if (FAL_OK != feature_api(FEATURE_API_ME_00000020_UPGRADE_CFG))
        {
            *pbFlashingRet = FALSE;

            goto out;
        }
    }

    if (OMCI_ME_CLASS_SOFTWARE_IMAGE == meClass)
    {
        // update image version base on the uboot flag
        sprintf(flagName, "sw_version%u", mibSoftwareImage.EntityID);
        if ((pStr = omci_sw_dl_uboot_env_get(flagName)))
        {
            snprintf(mibSoftwareImage.Version, sizeof(mibSoftwareImage.Version), "%s", pStr);

            free(pStr);
        }

        // flashing process is completely done, mark partition as valid
        mibSoftwareImage.Valid = TRUE;
        if (GOS_OK != MIB_Set(MIB_TABLE_SWIMAGE_INDEX, &mibSoftwareImage, sizeof(MIB_TABLE_SWIMAGE_T)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Modify image valid state from FALSE to TRUE fail");

            *pbFlashingRet = FALSE;

            goto out;
        }

        // rewrite the sw_validX according to the real instanceID
        sprintf(flagName, "sw_valid%u", mibSoftwareImage.EntityID);
        sprintf(flagBuffer, "%u", mibSoftwareImage.Valid);
        if (omci_sw_dl_uboot_env_set(flagName, flagBuffer))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Modify image valid state from FALSE to TRUE fail");

            // rollback the image valid state
            mibSoftwareImage.Valid = FALSE;
            MIB_Set(MIB_TABLE_SWIMAGE_INDEX, &mibSoftwareImage, sizeof(MIB_TABLE_SWIMAGE_T));

            *pbFlashingRet = FALSE;

            goto out;
        }
    }

    *pbFlashingRet = TRUE;

out:
    // update flashing state
    bFlashingState = FALSE;
    omci_sw_dl_flashing_state_updater(TRUE, &bFlashingState);

    return pbFlashingRet;
}

GOS_ERROR_CODE OMCI_OnEndSWDL(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    omci_me_class_t     classID         = (omci_me_class_t)pNorOmciMsg->meId.meClass;
    omci_me_instance_t  instanceID      = (omci_me_instance_t)pNorOmciMsg->meId.meInstance;

    MIB_TABLE_SWIMAGE_T mibSoftwareImage;
    UINT32              endImageByteSize, crc32;
    BOOL                bFlashingState;
    struct sched_param  sched;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Handling Action: End Software Download %x\n", instanceID);

    if (OMCI_ME_CLASS_SOFTWARE_IMAGE == classID)
    {
        // check if instance exists or not by using instanceID
        mibSoftwareImage.EntityID = instanceID;
        if (GOS_OK != MIB_Get(MIB_TABLE_SWIMAGE_INDEX, &mibSoftwareImage, sizeof(MIB_TABLE_SWIMAGE_T)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "InstanceID not exist in MIB, class=%d, instance=0x%x", classID, instanceID);

            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_UNKNOWN_ME_INSTANCE, TRUE);;
        }

        // return failure if any one of is_committed or is_active flag is true
        if (mibSoftwareImage.Committed || mibSoftwareImage.Active)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Software Download for instance being committed or active is prohibited");

            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PARAMETER_ERROR, TRUE);
        }
    }
    else
    {
        if (FAL_OK != feature_api(FEATURE_API_ME_00000020_IS_SUPPORTED, &classID))
            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_CMD_NOT_SUPPORTED, TRUE);
    }

    // retrieve crc32 and image byte size
    crc32 = (pNorOmciMsg->content[0] << 24) + (pNorOmciMsg->content[1] << 16) +
        (pNorOmciMsg->content[2] << 8) + pNorOmciMsg->content[3];
    endImageByteSize = (pNorOmciMsg->content[4] << 24) + (pNorOmciMsg->content[5] << 16) +
        (pNorOmciMsg->content[6] << 8) + pNorOmciMsg->content[7];

    // return failure when the image size is different between start and end
    if (endImageByteSize != gOmciSwDlCtrl.imageByteSize)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Image size is different between Start and End Software Download");

        return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PARAMETER_ERROR, TRUE);
    }

    if (!gOmciSwDlCtrl.bCrcCheckDone)
    {
        FILE    *pFD;
        UINT32  i, calcCrc32 = 0;
        UINT16  windowSizeInBytes;
        UINT16  readSizeInBytes;

        windowSizeInBytes = OMCI_SW_DL_MAX_BYTE_OF_SECTION_IN_BASELINE_MSG * (gOmciSwDlCtrl.windowSize + 1);

        // re-use the buffer that is allocated for download window handling
        if (!gOmciSwDlCtrl.pDlWindowBuffer)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "There is no memory available for Software Download handling");

            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
        }

        // check if the image file buffer is available or not
        pFD = fopen(OMCI_SW_DL_IMAGE_LOCATION, "r");
        if (!pFD)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "The image file buffer cannot be opened for reading");

            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
        }

        // calculate the crc from the image file buffer
        for (i = 0; i < (endImageByteSize / windowSizeInBytes); i++)
        {
            readSizeInBytes = fread(gOmciSwDlCtrl.pDlWindowBuffer, 1, windowSizeInBytes, pFD);

            calcCrc32 = omci_CalcCRC32(calcCrc32, gOmciSwDlCtrl.pDlWindowBuffer, readSizeInBytes);
        }
        if ((endImageByteSize % windowSizeInBytes) != 0)
        {
            readSizeInBytes = fread(gOmciSwDlCtrl.pDlWindowBuffer, 1, endImageByteSize % windowSizeInBytes, pFD);

            calcCrc32 = omci_CalcCRC32(calcCrc32, gOmciSwDlCtrl.pDlWindowBuffer, readSizeInBytes);
        }

        // release window buffer
        free(gOmciSwDlCtrl.pDlWindowBuffer);
        gOmciSwDlCtrl.pDlWindowBuffer = NULL;

        fclose(pFD);

        // return failure if the calculated crc is not matched to the received one
        if (calcCrc32 != crc32)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "The image file buffer crc is not corrected");

            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
        }

        // mark flashing state as true
        bFlashingState = TRUE;
        omci_sw_dl_flashing_state_updater(TRUE, &bFlashingState);

        if (pthread_create(&gOmciSwDlUpgradeProcedureThreadID, NULL, omci_sw_dl_upgrade_procedure_thread, &pNorOmciMsg->meId))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Create software upgrade thread fail");

            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
        }

        OMCI_TaskDelay(100);

        /*set priority*/
        sched.sched_priority = OMCI_TASK_PRI_MAIN - 1;
        if (0 != pthread_setschedparam(gOmciSwDlUpgradeProcedureThreadID, SCHED_RR, &sched))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "set thread's priority fail");
            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
        }

        OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Starting busy on flashing image...");

        // send out the response
        OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_DEVICE_BUSY, TRUE);

        gOmciSwDlCtrl.bCrcCheckDone = TRUE;
    }
    else
    {
        // update flashing state
        omci_sw_dl_flashing_state_updater(FALSE, &bFlashingState);

        // check if the flashing is finished or not
        if (bFlashingState)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Still busy on flashing image...");

            // send out the response
            OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_DEVICE_BUSY, TRUE);
        }
        else
        {
            BOOL    *pbFlashingRet = NULL;

            // if join thread fail, then the upgrade result cannot be read
            if (pthread_join(gOmciSwDlUpgradeProcedureThreadID, (void **)&pbFlashingRet))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Cannot join the sw upgrade thread for exit status");

                return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
            }

            // if the upgrade result cannot be read, then upgrade process must be failed
            if (!pbFlashingRet)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Cannot get the exit status from sw upgrade thread");

                return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
            }

            // check if the flashing is successful finished or not
            if (*pbFlashingRet)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Image flashing process done");

                // sync mib
                omci_SyncMibData();

                // send out the response
                OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSED_SUCCESSFULLY, TRUE);
            }
            else
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Image flashing process fail");

                // send out the response
                OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
            }

            free(pbFlashingRet);

            // remove the image file
            if(remove(OMCI_SW_DL_IMAGE_LOCATION)!=0){
                            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "IMAGE delete failed...\n");
                        }
        }
    }

    return GOS_OK;
}


GOS_ERROR_CODE OMCI_OnActivateSw(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    omci_me_class_t     classID         = (omci_me_class_t)pNorOmciMsg->meId.meClass;
    omci_me_instance_t  instanceID      = (omci_me_instance_t)pNorOmciMsg->meId.meInstance;
    MIB_TABLE_SWIMAGE_T mibSoftwareImage;
    char                flagName[OMCI_SW_DL_IMAGE_UBOOT_FLAG_NAME];
    char                flagBuffer[OMCI_SW_DL_IMAGE_UBOOT_FLAG_BUFFER];
    BOOL                shouldReboot = FALSE;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Handling Action: Activate Image %x\n", instanceID);

    if (OMCI_ME_CLASS_SOFTWARE_IMAGE == classID)
    {
        // check if instance exists or not by using instanceID
        mibSoftwareImage.EntityID = instanceID;
        if (GOS_OK != MIB_Get(MIB_TABLE_SWIMAGE_INDEX, &mibSoftwareImage, sizeof(MIB_TABLE_SWIMAGE_T)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "InstanceID not exist in MIB, class=%d, instance=0x%x", classID, instanceID);

            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_UNKNOWN_ME_INSTANCE, TRUE);;
        }

        // return failure if any is_valid flag is false
        if (!mibSoftwareImage.Valid)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Activate image for invalid instance is prohibited");

            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PARAMETER_ERROR, TRUE);
        }

        // activate the image by change uboot flag
        if (!mibSoftwareImage.Active)
        {
            sprintf(flagName, "%s", "sw_tryactive");
            sprintf(flagBuffer, "%u", instanceID);
            if (omci_sw_dl_uboot_env_set(flagName, flagBuffer))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Modify sw_tryactive uboot flag fail");

                return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
            }
            else
                shouldReboot = TRUE;
        }
    }
    else
    {
        if (FAL_OK != feature_api(FEATURE_API_ME_00000020_IS_SUPPORTED, &classID))
            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_CMD_NOT_SUPPORTED, TRUE);
        else
            shouldReboot = TRUE;
    }

    // sync mib
    omci_SyncMibData();

    OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSED_SUCCESSFULLY, TRUE);

    // leave the reboot action to the last
    if (shouldReboot)
    {
        OMCI_PRINT("[omci] receive sw activate, now rebooting...");

        OMCI_TaskDelay(100);

        system("echo 1 > /proc/load_reboot");
    }

    return GOS_OK;
}

GOS_ERROR_CODE OMCI_OnCommitSw(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    omci_me_class_t     classID         = (omci_me_class_t)pNorOmciMsg->meId.meClass;
    omci_me_instance_t  instanceID      = (omci_me_instance_t)pNorOmciMsg->meId.meInstance;
    MIB_TABLE_SWIMAGE_T mibSoftwareImage;
    GOS_ERROR_CODE      ret;
    char                flagName[OMCI_SW_DL_IMAGE_UBOOT_FLAG_NAME];
    char                flagBuffer[OMCI_SW_DL_IMAGE_UBOOT_FLAG_BUFFER];
    BOOL                shouldReboot = FALSE;
    UINT8               isActive;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Handling Action: Commit Image %x\n", instanceID);

    if (OMCI_ME_CLASS_SOFTWARE_IMAGE == classID)
    {
        // check if instance exists or not by using instanceID
        mibSoftwareImage.EntityID = instanceID;
        if (GOS_OK != MIB_Get(MIB_TABLE_SWIMAGE_INDEX, &mibSoftwareImage, sizeof(MIB_TABLE_SWIMAGE_T)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "InstanceID not exist in MIB, class=%d, instance=0x%x", classID, instanceID);

            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_UNKNOWN_ME_INSTANCE, TRUE);;
        }

        // return failure if any is_valid flag is false
        if (!mibSoftwareImage.Valid)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Commit image for invalid instance is prohibited");

            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PARAMETER_ERROR, TRUE);
        }

        // store the active state
        isActive = mibSoftwareImage.Active;

        mibSoftwareImage.Committed = TRUE;
        if (GOS_OK != MIB_Set(MIB_TABLE_SWIMAGE_INDEX, &mibSoftwareImage, sizeof(MIB_TABLE_SWIMAGE_T)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Commit image fail at writing data into mib database");

            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
        }

        ret = MIB_GetFirst(MIB_TABLE_SWIMAGE_INDEX, &mibSoftwareImage, sizeof(MIB_TABLE_SWIMAGE_T));
        while (GOS_OK == ret)
        {
            if (instanceID != mibSoftwareImage.EntityID)
            {
                mibSoftwareImage.Committed = FALSE;
                if (GOS_OK != MIB_Set(MIB_TABLE_SWIMAGE_INDEX, &mibSoftwareImage, sizeof(MIB_TABLE_SWIMAGE_T)))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Commit image fail at writing data into mib database");

                    return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
                }
            }

            ret = MIB_GetNext(MIB_TABLE_SWIMAGE_INDEX, &mibSoftwareImage, sizeof(MIB_TABLE_SWIMAGE_T));
        }

        // making commit affected later if necessary
        if (isActive)
        {
            sprintf(flagName, "%s", "sw_commit");
            sprintf(flagBuffer, "%u", instanceID);
            if (omci_sw_dl_uboot_env_set(flagName, flagBuffer))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Modify sw_commit uboot flag fail");

                return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
            }
        }
        else
        {
            sprintf(flagName, "%s", "sw_commit_later");
            sprintf(flagBuffer, "%u", instanceID);
            if (omci_sw_dl_uboot_env_set(flagName, flagBuffer))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Modify sw_commit_later uboot flag fail");

                return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
            }

            sprintf(flagName, "%s", "sw_tryactive");
            sprintf(flagBuffer, "%u", instanceID);
            if (omci_sw_dl_uboot_env_set(flagName, flagBuffer))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Modify sw_tryactive uboot flag fail");

                return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSING_ERROR, TRUE);
            }
        }
    }
    else
    {
        if (FAL_OK != feature_api(FEATURE_API_ME_00000020_IS_SUPPORTED, &classID))
            return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_CMD_NOT_SUPPORTED, TRUE);
        else
            shouldReboot = TRUE;
    }

    // sync mib
    omci_SyncMibData();

    OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSED_SUCCESSFULLY, TRUE);

    // leave the reboot action to the last
    if (shouldReboot)
    {
        OMCI_PRINT("[omci] receive sw commit, now rebooting...");

        OMCI_TaskDelay(100);

        system("echo 1 > /proc/load_reboot");
    }

    return GOS_OK;
}

GOS_ERROR_CODE OMCI_OnSync(omci_msg_norm_baseline_t     *pNorOmciMsg)
{
    BOOL            bTimeSet = FALSE;
    BOOL            bSyncState = TRUE;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Handling Action: Sync");

    // sync system date and time
    if (GOS_OK != omci_util_is_allZero(pNorOmciMsg->content, OMCI_MSG_BASELINE_CONTENTS_LEN))
    {
        struct timespec     ts;
        struct tm           tm;

        memset(&ts, 0, sizeof(ts));
        memset(&tm, 0, sizeof(tm));
        tm.tm_year  = GOS_Htons(((UINT16)pNorOmciMsg->content[0] << 8 | pNorOmciMsg->content[1]) -1900);
        tm.tm_mon   = pNorOmciMsg->content[2] - 1;
        tm.tm_mday  = pNorOmciMsg->content[3];
        tm.tm_hour  = pNorOmciMsg->content[4];
        tm.tm_min   = pNorOmciMsg->content[5];
        tm.tm_sec   = pNorOmciMsg->content[6];

        // translate struct tm into time_t
        if (-1 != (ts.tv_sec = mktime(&tm)))
        {
            if (!clock_settime(CLOCK_REALTIME, &ts))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                    "System clock has been regulated by OMCI");

                bTimeSet = TRUE;
            }
        }
    }

    // sync pm tick boundary
    omci_pm_sync_request_updater(TRUE, &bSyncState);

    // prepare response reault
    memset(pNorOmciMsg->content, 0, OMCI_MSG_BASELINE_CONTENTS_LEN);
    pNorOmciMsg->content[1] = bTimeSet;

    return OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSED_SUCCESSFULLY, FALSE);
}


GOS_ERROR_CODE OMCI_OnReboot(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    if (FAL_OK == feature_api(FEATURE_API_ME_00000800_IS_ENABLED,
            &pNorOmciMsg->meId.meClass, &pNorOmciMsg->meId.meInstance))
    {
        feature_api(FEATURE_API_ME_00000800_DRV_CFG, pNorOmciMsg);
    }
    else
    {
        OMCI_ResponseMsg(pNorOmciMsg, OMCI_RSP_PROCESSED_SUCCESSFULLY, TRUE);

        OMCI_PRINT("[omci] receive reboot, now rebooting...");

        OMCI_TaskDelay(100);

        system("echo 1 > /proc/load_reboot");
    }

    return GOS_OK;
}

GOS_ERROR_CODE OMCI_OnMibUploadMsg(omci_msg_norm_baseline_t* pNorOmciMsg)
{

    omci_FsmRun(gOmciMibUploadFsm, OMCI_MIB_UPLOAD_EVT_START, pNorOmciMsg);

#ifndef OMCI_X86
    if(gInfo.devIdVersion.chipId == APOLLOMP_CHIP_ID)
    {
        //omci_pon_bw_threshold_t ponBwThreshold;
        /*Enlarge PON threshold. */
        /*Becasue Huawei OLT set small bwmap to OMCC when ONU unauthenticated,
          so set PON threshold small at begin, and enlarge threshold when ONU authenticated*/
        //ponBwThreshold.bwThreshold = 0x15;
        //ponBwThreshold.reqBwThreshold = 0x16;
        // user-space connection mgr will take care
        // omci_wrapper_setPonBwThreshold(&ponBwThreshold);
    }
#endif
    return GOS_OK;
}

GOS_ERROR_CODE OMCI_OnMibUploadNextMsg(omci_msg_norm_baseline_t* pNorOmciMsg)
{
    omci_FsmRun(gOmciMibUploadFsm, OMCI_MIB_UPLOAD_EVT_GET, pNorOmciMsg);
    return GOS_OK;
}


BOOL OMCI_CheckIsActionSupported(UINT16 meClassID, UINT8 type)
{
    MIB_TABLE_INDEX tableIndex;
    omci_me_action_mask_t actionType = 0;


    tableIndex = MIB_GetTableIndexByClassId(meClassID);

    switch(type)
    {
        case OMCI_MSG_TYPE_CREATE:
        {
            actionType = OMCI_ME_ACTION_CREATE;
            break;
        }
        case OMCI_MSG_TYPE_DELETE:
        {
            actionType = OMCI_ME_ACTION_DELETE;
            break;
        }
        case OMCI_MSG_TYPE_SET:
        {
            actionType = OMCI_ME_ACTION_SET;
            break;
        }
        case OMCI_MSG_TYPE_GET:
        {
            actionType = OMCI_ME_ACTION_GET;
            break;
        }
        case OMCI_MSG_TYPE_GET_ALL_ALARMS:
        {
            actionType = OMCI_ME_ACTION_GET_ALL_ALARMS;
            break;
        }
        case OMCI_MSG_TYPE_GET_ALL_ALARMS_NEXT:
        {
            actionType = OMCI_ME_ACTION_GET_ALL_ALARMS_NEXT;
            break;
        }
        case OMCI_MSG_TYPE_MIB_UPLOAD:
        {
            actionType = OMCI_ME_ACTION_MIB_UPLOAD;
            break;
        }
        case OMCI_MSG_TYPE_MIB_UPLOAD_NEXT:
        {
            actionType = OMCI_ME_ACTION_MIB_UPLOAD_NEXT;
            break;
        }
        case OMCI_MSG_TYPE_MIB_RESET:
        {
            actionType = OMCI_ME_ACTION_MIB_RESET;
            break;
        }
        case OMCI_MSG_TYPE_ACTIVATE_SW:
        {
            actionType = OMCI_ME_ACTION_ACTIVATE_SW;
            break;
        }
        case OMCI_MSG_TYPE_COMMIT_SW:
        {
            actionType = OMCI_ME_ACTION_COMMIT_SW;
            break;
        }
        case OMCI_MSG_TYPE_REBOOT:
        {
            actionType = OMCI_ME_ACTION_REBOOT;
            break;
        }
        case OMCI_MSG_TYPE_GET_NEXT:
        {
            actionType = OMCI_ME_ACTION_GET_NEXT;
            break;
        }
        case OMCI_MSG_TYPE_START_SW_DOWNLOAD:
        {
            actionType = OMCI_ME_ACTION_START_SW_DOWNLOAD;
            break;
        }
        case OMCI_MSG_TYPE_DOWNLOAD_SECTION:
        {
            actionType = OMCI_ME_ACTION_DOWNLOAD_SECTION;
            break;
        }
        case OMCI_MSG_TYPE_END_SW_DOWNLOAD:
        {
            actionType = OMCI_ME_ACTION_END_SW_DOWNLOAD;
            break;
        }
        case OMCI_MSG_TYPE_SYNCHRONIZE_TIME:
        {
            actionType = OMCI_ME_ACTION_SYNCHRONIZE_TIME;
            break;
        }
        case OMCI_MSG_TYPE_GET_CURRENT_DATA:
        {
            actionType = OMCI_ME_ACTION_GET_CURRENT_DATA;
            break;
        }
        case OMCI_MSG_TYPE_TEST:
        {
            actionType = OMCI_ME_ACTION_TEST;
            break;
        }
        default:
            return FALSE;
    }

    return MIB_TableSupportAction(tableIndex, actionType);
}

UINT16 OMCI_GetSeqNumPerRow_wrapper(MIB_TABLE_INDEX tableIndex)
{
    return omci_GetSeqNumPerRow(tableIndex);
}

