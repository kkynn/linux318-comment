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
 * Purpose : Definition of OMCI MIB related APIs
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) OMCI MIB related APIs
 */

#include "mib_table.h"
#include "omci_util.h"
#include "feature_mgmt.h"


#ifdef OMCI_X86
#define MIB_TABLE_MODULE_PATH "MIB/src/tables/"
#else
#define MIB_TABLE_MODULE_PATH "/lib/omci/"
#endif


extern MIB_FOREST_T forest;
extern omciMibTables_t gTables;

extern int MIB_TABLE_LAST_INDEX;
extern int MIB_TABLE_TOTAL_NUMBER;

typedef struct 
mib_dlhndl_s {
    unsigned* mibDlHd;
    struct mib_dlhndl_s* next;
} mib_dlhndl_t;

static mib_dlhndl_t* mibDlHndlTbl = NULL;


MIB_TABLE_INFO_T* MIB_GetTableInfoPtr(MIB_TABLE_INDEX tableIndex)
{
	MIB_TABLE_T *pTable;
	pTable = mib_GetTablePtr(tableIndex);

	if(pTable)
	{
 		return pTable->pTableInfo;
    }
    return NULL;
}


void* MIB_GetDefaultRow(MIB_TABLE_INDEX tableIndex)
{

	MIB_TABLE_INFO_T *pTableInfo;

	pTableInfo = MIB_GetTableInfoPtr(tableIndex);

	if(pTableInfo)
	{
 		return pTableInfo->pDefaultRow;
    }
    return NULL;
}



const CHAR* MIB_GetTableName(MIB_TABLE_INDEX tableIndex)
{
    MIB_TABLE_INFO_T* pTableInfo = MIB_GetTableInfoPtr(tableIndex);
    GOS_ASSERT(pTableInfo != NULL);
    return pTableInfo->Name;
}

const CHAR* MIB_GetTableShortName(MIB_TABLE_INDEX tableIndex)
{
    MIB_TABLE_INFO_T* pTableInfo = MIB_GetTableInfoPtr(tableIndex);
    GOS_ASSERT(pTableInfo != NULL);
    return pTableInfo->ShortName;
}


const CHAR* MIB_GetTableDesc(MIB_TABLE_INDEX tableIndex)
{
    MIB_TABLE_INFO_T* pTableInfo = MIB_GetTableInfoPtr(tableIndex);
    GOS_ASSERT(pTableInfo != NULL);
    return pTableInfo->Desc;
}


UINT32 MIB_GetTableClassId(MIB_TABLE_INDEX tableIndex)
{
    MIB_TABLE_INFO_T* pTableInfo = MIB_GetTableInfoPtr(tableIndex);
    GOS_ASSERT(pTableInfo != NULL);
    return pTableInfo->ClassId;
}


UINT32 MIB_GetTableInitType(MIB_TABLE_INDEX tableIndex)
{
    MIB_TABLE_INFO_T* pTableInfo = MIB_GetTableInfoPtr(tableIndex);
    GOS_ASSERT(pTableInfo != NULL);
    return pTableInfo->InitType;
}


UINT32 MIB_GetTableStdType(MIB_TABLE_INDEX tableIndex)
{
    MIB_TABLE_INFO_T* pTableInfo = MIB_GetTableInfoPtr(tableIndex);
    GOS_ASSERT(pTableInfo != NULL);
    return pTableInfo->StdType;
}


UINT32 MIB_GetTableActionType(MIB_TABLE_INDEX tableIndex)
{
    MIB_TABLE_INFO_T* pTableInfo = MIB_GetTableInfoPtr(tableIndex);
    GOS_ASSERT(pTableInfo != NULL);
    return pTableInfo->ActionType;
}



const CHAR* MIB_GetAttrName(MIB_TABLE_INDEX tableIndex, MIB_ATTR_INDEX attrIndex)
{
    MIB_ATTR_INFO_T* pAttrInfo = mib_GetAttrInfo(tableIndex, attrIndex);
    GOS_ASSERT(pAttrInfo != NULL);
    return pAttrInfo->Name;
}


const CHAR* MIB_GetAttrDesc(MIB_TABLE_INDEX tableIndex, MIB_ATTR_INDEX attrIndex)
{
    MIB_ATTR_INFO_T* pAttrInfo = mib_GetAttrInfo(tableIndex, attrIndex);
    GOS_ASSERT(pAttrInfo != NULL);
    return pAttrInfo->Desc;
}


MIB_ATTR_TYPE MIB_GetAttrDataType(MIB_TABLE_INDEX tableIndex, MIB_ATTR_INDEX attrIndex)
{
    MIB_ATTR_INFO_T* pAttrInfo = mib_GetAttrInfo(tableIndex, attrIndex);
    GOS_ASSERT(pAttrInfo != NULL);
    return pAttrInfo->DataType;
}


UINT32 MIB_GetAttrLen(MIB_TABLE_INDEX tableIndex, MIB_ATTR_INDEX attrIndex)
{
    MIB_ATTR_INFO_T* pAttrInfo = mib_GetAttrInfo(tableIndex, attrIndex);
    GOS_ASSERT(pAttrInfo != NULL);
    return pAttrInfo->Len;
}


BOOL MIB_GetAttrIsIndex(MIB_TABLE_INDEX tableIndex, MIB_ATTR_INDEX attrIndex)
{
    MIB_ATTR_INFO_T* pAttrInfo = mib_GetAttrInfo(tableIndex, attrIndex);
    GOS_ASSERT(pAttrInfo != NULL);
    return pAttrInfo->IsIndex;
}


BOOL MIB_GetAttrMibSave(MIB_TABLE_INDEX tableIndex, MIB_ATTR_INDEX attrIndex)
{
    MIB_ATTR_INFO_T* pAttrInfo = mib_GetAttrInfo(tableIndex, attrIndex);
    GOS_ASSERT(pAttrInfo != NULL);
    return pAttrInfo->MibSave;
}


MIB_ATTR_OUT_STYLE MIB_GetAttrOutStyle(MIB_TABLE_INDEX tableIndex, MIB_ATTR_INDEX attrIndex)
{
    MIB_ATTR_INFO_T* pAttrInfo = mib_GetAttrInfo(tableIndex, attrIndex);
    GOS_ASSERT(pAttrInfo != NULL);
    return pAttrInfo->OutStyle;
}


UINT32 MIB_GetAttrOltAcc(MIB_TABLE_INDEX tableIndex, MIB_ATTR_INDEX attrIndex)
{
    MIB_ATTR_INFO_T* pAttrInfo = mib_GetAttrInfo(tableIndex, attrIndex);
    GOS_ASSERT(pAttrInfo != NULL);
    return pAttrInfo->OltAcc;
}


BOOL MIB_GetAttrAvcFlag(MIB_TABLE_INDEX tableIndex, MIB_ATTR_INDEX attrIndex)
{
    MIB_ATTR_INFO_T* pAttrInfo = mib_GetAttrInfo(tableIndex, attrIndex);
    GOS_ASSERT(pAttrInfo != NULL);
    return pAttrInfo->AvcFlag;
}


UINT32 MIB_GetAttrOptionType(MIB_TABLE_INDEX tableIndex, MIB_ATTR_INDEX attrIndex)
{
    MIB_ATTR_INFO_T* pAttrInfo = mib_GetAttrInfo(tableIndex, attrIndex);
    GOS_ASSERT(pAttrInfo != NULL);
    return pAttrInfo->OptionType;
}



UINT32 MIB_GetTableAttrNum(MIB_TABLE_INDEX tableIndex)
{

	MIB_TABLE_INFO_T *pTableInfo;

	pTableInfo = MIB_GetTableInfoPtr(tableIndex);
	if(pTableInfo)
	{
    	return pTableInfo->attrNum;
	}
	return 0;
}


UINT32 MIB_GetTableEntrySize(MIB_TABLE_INDEX tableIndex)
{

	MIB_TABLE_INFO_T *pTableInfo;

	pTableInfo = MIB_GetTableInfoPtr(tableIndex);

	if(pTableInfo)
	{
    	return pTableInfo->entrySize;
	}
    return 0;
}

void MIB_Foreset_Init(void)
{
	LIST_INIT(&forest.treeHead);
	forest.treeCount  = 0;
}

static
UINT32
MIB_DlHndlAdd ( 
    unsigned* hndl
)
{
    UINT32 ret = 0;
    mib_dlhndl_t* pHndlNd;

    if (!hndl)
    {
        OMCI_LOG( OMCI_LOG_LEVEL_ERR, "%s: hndl is null\n", __FUNCTION__ )
        goto ERR_MIB_DLHNDLADD;
    }

    pHndlNd = ( mib_dlhndl_t* ) malloc ( sizeof ( mib_dlhndl_t ) );
    if (!pHndlNd)
    {
        OMCI_LOG( OMCI_LOG_LEVEL_ERR, "%s: Failed to allocate space for dlHndl\n", __FUNCTION__ )
        goto ERR_MIB_DLHNDLADD;
    }

    pHndlNd->mibDlHd = hndl;
    pHndlNd->next = mibDlHndlTbl;
    mibDlHndlTbl = pHndlNd;
    ret = 1;
    
 ERR_MIB_DLHNDLADD:
    return ret;
}

static
UINT32
MIB_DlHndlDelAll(
    void
    )
{
    UINT32 ret = 0;
    mib_dlhndl_t* cur = NULL;

    if (!mibDlHndlTbl)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s: mibDlHndlTbl is null\n", __FUNCTION__)
        goto ERR_MIB_DLHNDLDEL;
    }

    cur = mibDlHndlTbl;
    while (cur)
    {
        if (cur->mibDlHd)
        {
            dlclose(cur->mibDlHd);
        }
        cur = cur->next;
        free(mibDlHndlTbl);
        mibDlHndlTbl = cur;
    }
    ret = 1;

ERR_MIB_DLHNDLDEL:
    return ret;
}

void
MIB_Table_Init(void)
{
    int ret = 0;
    DIR *dir = NULL;
    void *handle = NULL;
    struct dirent *ptr = NULL;
    struct stat fileStat;
    char strModuleFile[256] = "";
    int tableId;
    GOS_ERROR_CODE (*mibTable_init)(int mibTableId);
    
    if((dir = opendir(MIB_TABLE_MODULE_PATH))){
        OMCI_LOG(OMCI_LOG_LEVEL_INFO,"%s open dir:%s to serach omci mib tables!",__FUNCTION__,MIB_TABLE_MODULE_PATH);
        while((ptr = readdir(dir)) != NULL){
            sprintf(strModuleFile, "%s%s", MIB_TABLE_MODULE_PATH, ptr->d_name);
            if(stat(strModuleFile, &fileStat)<0){
                continue;
            }
            if(!S_ISREG(fileStat.st_mode)){
                /* Not a file */
                continue;
            }
            /*check .so file only*/
            if(!strstr(strModuleFile,".so")){
                /*Not a share library*/
                continue;
            }
            OMCI_LOG(OMCI_LOG_LEVEL_INFO,"%s find mibTable:%s",__FUNCTION__,strModuleFile);
            dlerror(); /* Clear any existing error */
            handle = dlopen(strModuleFile, RTLD_LAZY);
            if(!handle){
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,"%s [Error]: Can't load omci mib table:%s",__FUNCTION__,ptr->d_name);
                continue;
            }

            mibTable_init = dlsym(handle, "mibTable_init");
            if(!mibTable_init) {
                OMCI_LOG(OMCI_LOG_LEVEL_WARN,"%s [Error]:%s doesn't implement omci MibTable_Init function",__FUNCTION__,strModuleFile);
                dlclose(handle); 
                continue;
            }
            OMCI_LOG(OMCI_LOG_LEVEL_INFO,"%s Loading mib table: %s...",__FUNCTION__,strModuleFile);

            tableId = MIB_Register();
            if(tableId == MIB_TABLE_UNKNOWN_INDEX)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "mib cfg: Get tblIdx failed, mib table: %s...", strModuleFile);
                dlclose(handle); 
                continue;
            }
            
            ret = mibTable_init(tableId);

            if(ret !=0)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,"%s Init mib table:%s fail, error code is:%d...",__FUNCTION__,ptr->d_name, ret);
                MIB_UnRegister((MIB_TABLE_INDEX)tableId);
                dlclose (handle);
                continue;
            }
            //
            // Save the dl handle of MIB in mibDlHdTbl.
            //
            MIB_DlHndlAdd ( handle );
        }
        closedir(dir);
        MIB_TABLE_TOTAL_NUMBER  = MIB_GetTableNum()-1;
        MIB_TABLE_LAST_INDEX    = MIB_TABLE_TOTAL_NUMBER;

        OMCI_LOG(OMCI_LOG_LEVEL_INFO,"End of search  OMCI tables!, tables number %d",MIB_TABLE_TOTAL_NUMBER);
    }
    else
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Can't find OMCI tables path %s!",MIB_TABLE_MODULE_PATH);
    }

    if (FAL_ERR_NOT_REGISTER == feature_api_is_registered(FEATURE_API_ME_00040000))
    {
        MIB_TableSortingByTableIndex();
    }
    else
    {
        if (FAL_OK != feature_api(FEATURE_API_ME_00040000))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "MIB Table Sorting failed...");
        }
    }
}

void
MIB_Table_Deinit(void)
{

    MIB_DlHndlDelAll (  );

    return;
}


GOS_ERROR_CODE
MIB_Init(MIB_TABLE_INDEX* pIncludeTbl, MIB_TABLE_INDEX* pExcludeTbl)
{

	/*create tables*/

	LIST_INIT(&gTables.head);
	gTables.tablesCount = MIB_TABLE_FIRST_INDEX;

	/*init tree*/
	MIB_Foreset_Init();

	/*init to default ME data*/
	MIB_Table_Init();
    
    return GOS_OK;
}

GOS_ERROR_CODE
MIB_Deinit ( 
    void )
{
    MIB_Table_Deinit ();
    return GOS_OK;
}
