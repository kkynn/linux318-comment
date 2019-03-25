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
#include "omci_timer.h"
#include "omci_task.h"
#include "omci_internal_api.h"
#include "omci_driver_api.h"
#ifndef OMCI_X86
#include "rtk/svlan.h"
#endif
#include "feature_mgmt.h"

MIB_FOREST_T forest;
struct connHead connsInCfg = LIST_HEAD_INITIALIZER(connsInCfg);

/* Tree Connection configuration task */
static OMCI_TASK_INFO_T *pOperTask;

/* Mutex protection */
pthread_mutex_t gOmciTreeCfgMutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct WORK_MSG_HDR_T{
    long msgType;
    long dummy;
} WORK_MSG_HDR_T;

/*for AVL tree*/
static MIB_NODE_T* MIB_AvlTreeRotateLL(MIB_NODE_T *parent)
{
    MIB_NODE_T *child = parent->lChild;
    parent->lChild = child->rChild;
    child->rChild = parent;
    return child;
}


static MIB_NODE_T* MIB_AvlTreeRotateRR(MIB_NODE_T *parent)
{
    MIB_NODE_T *child = parent->rChild;
    parent->rChild= child->lChild;
    child->lChild = parent;
    return child;
}


static MIB_NODE_T* MIB_AvlTreeRotateRL(MIB_NODE_T *parent)
{
    MIB_NODE_T *child = parent->rChild;
    parent->rChild= MIB_AvlTreeRotateLL(child);
    return MIB_AvlTreeRotateRR(parent);
}


static MIB_NODE_T* MIB_AvlTreeRotateLR(MIB_NODE_T *parent)
{
    MIB_NODE_T *child = parent->lChild;
    parent->lChild= MIB_AvlTreeRotateRR(child);
    return MIB_AvlTreeRotateLL(parent);
}

static int MIB_GetAvlTreeHeight(MIB_NODE_T *node)
{
    int height=0;
    if(node != NULL)
        height = 1+avlTreeGetMax(MIB_GetAvlTreeHeight(node->lChild),MIB_GetAvlTreeHeight(node->rChild));
    return height;
}


static int MIB_GetAvlTreeBalance(MIB_NODE_T *node)
{
    if(node == NULL) return 0;
    return MIB_GetAvlTreeHeight(node->lChild) - MIB_GetAvlTreeHeight(node->rChild);
}


MIB_NODE_T* MIB_BalanceAvlTree(MIB_NODE_T **node)
{
    int height_diff= MIB_GetAvlTreeBalance(*node);

    if(height_diff > 1)
{
        if(MIB_GetAvlTreeBalance((*node)->lChild) > 0)
            *node = MIB_AvlTreeRotateLL(*node);
        else
            *node = MIB_AvlTreeRotateLR(*node);
    }
    else if(height_diff < -1)
{
        if(MIB_GetAvlTreeBalance((*node)->rChild) < 0)
            *node = MIB_AvlTreeRotateRR(*node);
        else
            *node = MIB_AvlTreeRotateRL(*node);
    }
    return *node;
}


static MIB_NODE_T* mib_avlTreeNodeAdd(MIB_NODE_T **root,MIB_AVL_KEY_T key, MIB_TABLE_INDEX tableId,void *pData)
{
    MIB_ENTRY_T     *pEntry = NULL;
    MIB_TREE_DATA_T *treeData = NULL;
    MIB_TREE_NODE_ENTRY_T *pTreeDataEntry = NULL;
    UINT16          id;

    if(*root == NULL)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: find NULL node",__FUNCTION__);
        *root = (MIB_NODE_T*)malloc(sizeof(MIB_NODE_T));
        if(*root == NULL)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,"fail: memory allocation");
            return NULL;
        }
        treeData = &((*root)->data);
        treeData->key = key;
        treeData->tableIndex = tableId;
        LIST_INIT(&treeData->treeNodeEntryHead);
        pTreeDataEntry = (MIB_TREE_NODE_ENTRY_T*)malloc(sizeof(MIB_TREE_NODE_ENTRY_T));
        LIST_INSERT_HEAD(&treeData->treeNodeEntryHead,pTreeDataEntry,treeNodeEntry);

        /*Search Entry from MIB*/
        if((pEntry = MIB_GetTableEntry(tableId,pData))==NULL)
        {
             OMCI_LOG(OMCI_LOG_LEVEL_DBG,"fail for GetTable Entry!");
             return NULL;
        }

        MIB_GetAttrFromBuf(tableId,MIB_ATTR_FIRST_INDEX,&id,pData,sizeof(id));
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s(): Add Table Id: %d, Entity Id 0x%2x to entry(%x)",__FUNCTION__,tableId,id,(unsigned int)pEntry);
        pTreeDataEntry->mibEntry = pEntry;
        (*root)->lChild = (*root)->rChild = NULL;

    }
    else if(key < (*root)->data.key)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: Search left child, node key %d, insert key %d",__FUNCTION__,(*root)->data.key,key);
        (*root)->lChild = mib_avlTreeNodeAdd(&((*root)->lChild),key,tableId,pData);
        (*root) = MIB_BalanceAvlTree(root);
    }
    else if(key > (*root)->data.key)
    {

        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: Search right child, node key %d, insert key %d",__FUNCTION__,(*root)->data.key,key);
        (*root)->rChild = mib_avlTreeNodeAdd(&((*root)->rChild),key,tableId,pData);
        (*root) = MIB_BalanceAvlTree(root);
    }
    else
    {

        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: key is exist, add entry to list",__FUNCTION__);
        treeData = &(*root)->data;

        LIST_FOREACH(pTreeDataEntry,&treeData->treeNodeEntryHead,treeNodeEntry)
        {
            if(mib_CompareEntry(tableId,pData,pTreeDataEntry->mibEntry->pData)==0)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: find a duplicate entity Id.",__FUNCTION__);
                return *root;
            }
        }
        /*Search Entry from MIB*/
        if((pEntry=MIB_GetTableEntry(tableId,pData))==NULL)
        {
             OMCI_LOG(OMCI_LOG_LEVEL_DBG,"fail for GetTable Entry!");
             return NULL;
        }
        /*Add tree data entry*/
        pTreeDataEntry = (MIB_TREE_NODE_ENTRY_T*)malloc(sizeof(MIB_TREE_NODE_ENTRY_T));
        LIST_INSERT_HEAD(&treeData->treeNodeEntryHead,pTreeDataEntry,treeNodeEntry);
        pTreeDataEntry->mibEntry = pEntry;
    }

    return *root;
}

MIB_NODE_T* MIB_AvlTreeNodeAdd(MIB_NODE_T **root,MIB_AVL_KEY_T key, MIB_TABLE_INDEX tableId,void *pData)
{
    MIB_NODE_T *pNode;

    pthread_mutex_lock(&gOmciTreeCfgMutex);
    pNode = mib_avlTreeNodeAdd(root, key, tableId, pData);
    pthread_mutex_unlock(&gOmciTreeCfgMutex);

    return pNode;
}


MIB_NODE_T* MIB_AvlTreeSearch(MIB_NODE_T *node, MIB_AVL_KEY_T key)
{
    if(node == NULL) return NULL;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s(): %d->",__FUNCTION__,node->data.key);

    if(key == node->data.key)
        return node;
    else if(key < node->data.key)
        return MIB_AvlTreeSearch(node->lChild,key);
    else
        return MIB_AvlTreeSearch(node->rChild,key);
}


MIB_TREE_NODE_ENTRY_T* MIB_AvlTreeEntrySearch(MIB_NODE_T *node, MIB_AVL_KEY_T key,omci_me_instance_t entityId)
{
    MIB_ENTRY_T *pEntry;
    MIB_TREE_NODE_ENTRY_T *pNodeEntry;
    MIB_TREE_DATA_T     *pData;
    omci_me_instance_t  id ;
    int ismatch = 0;

    if(node == NULL) return NULL;

    pData = &node->data;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s(): Current key %d, Search key %d",__FUNCTION__,pData->key,key);

    if(key == pData->key)
    {
        LIST_FOREACH(pNodeEntry,&pData->treeNodeEntryHead,treeNodeEntry)
        {
            pEntry = pNodeEntry->mibEntry;
            MIB_GetAttrFromBuf(pData->tableIndex,MIB_ATTR_FIRST_INDEX,&id,pEntry->pData,sizeof(omci_me_instance_t));
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Entry (%x) entity id =0x%02x, Search Id 0x%02x,\n",(unsigned int)pEntry,id,entityId);
            if(id == entityId)
            {
                ismatch = 1;
                break;
            }
        }
        if(ismatch){
            return pNodeEntry;
        }else
        {
            return NULL;
        }
    }
    else if(key < pData->key)
        return MIB_AvlTreeEntrySearch(node->lChild,key,entityId);
    else
        return MIB_AvlTreeEntrySearch(node->rChild,key,entityId);
}


/*If pTreePrev == NULL, will search from treeHead.
  If pTreePrev != NULL, will search from next tree of pTreePrev*/
MIB_TREE_T* MIB_AvlTreeSearchByKey(MIB_TREE_T* pTreePrev, omci_me_instance_t entityId,MIB_AVL_KEY_T key)
{
    MIB_TREE_T *pTree, *pStartTree;
    MIB_TREE_NODE_ENTRY_T *pNodeEntry;

    if(forest.treeCount <= 0)
    {
        return NULL;
    }

    if(pTreePrev == NULL)
    {
        pStartTree = LIST_FIRST(&forest.treeHead);
    }else{
        pStartTree = LIST_NEXT(pTreePrev, entries);
    }

    for (pTree = pStartTree; pTree != NULL; pTree = LIST_NEXT(pTree,entries) )
    {
        pNodeEntry = MIB_AvlTreeEntrySearch(pTree->root,key,entityId);
        if(pNodeEntry!=NULL){
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: find pNodeEntry %p pEntry %p\n",__FUNCTION__, pNodeEntry, pNodeEntry->mibEntry);
            return pTree;
        }
    }

    return NULL;
}



GOS_ERROR_CODE MIB_AvlTreeEntryDump ( MIB_NODE_T* pNode )
{
    MIB_ENTRY_T* pEntry;
    MIB_TREE_NODE_ENTRY_T* pNodeEntry;
    omci_me_instance_t id;

    if(!pNode)
    {
        return GOS_FAIL;
    }

    /*preorder dump*/
    if(pNode->lChild != NULL)
    {
        MIB_AvlTreeEntryDump(pNode->lChild);
    }

    OMCI_PRINT("AVL Key:%d, TableName: %s", pNode->data.key, MIB_GetTableName(pNode->data.tableIndex));
    LIST_FOREACH(pNodeEntry, &pNode->data.treeNodeEntryHead, treeNodeEntry)
    {
        pEntry = pNodeEntry->mibEntry;
        MIB_GetAttrFromBuf(pNode->data.tableIndex, MIB_ATTR_FIRST_INDEX, &id, pEntry->pData, sizeof(id));
        OMCI_PRINT("Entity (%x) Id: 0x%02x", (unsigned int)pEntry, id);
    }

    if(pNode->rChild != NULL)
    {
        MIB_AvlTreeEntryDump(pNode->rChild);
    }
    return GOS_OK;
}


GOS_ERROR_CODE MIB_AvlTreeDump(void)
{
    MIB_TREE_T* pTree;
    int i=0;

    if(forest.treeCount > 0)
    {
        LIST_FOREACH(pTree,&forest.treeHead,entries)
        {
            OMCI_PRINT("AVL Tree ID: %d",i);
            MIB_AvlTreeEntryDump(pTree->root);
            i++;
        }
    }

    return GOS_OK;
}

GOS_ERROR_CODE MIB_AvlTreeDumpByKey(MIB_AVL_KEY_T key)
{
    MIB_TREE_T* pTree;
    MIB_NODE_T  *pNode;
    MIB_TREE_NODE_ENTRY_T *pNodeEntry;
    MIB_ENTRY_T *pEntry;
    omci_me_instance_t id;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Start %s...key=%u",__FUNCTION__, key);

    if(forest.treeCount > 0)
    {
        LIST_FOREACH(pTree,&forest.treeHead,entries)
        {

            pNode = MIB_AvlTreeSearch(pTree->root, key);
            if(!pNode)
            {
                OMCI_PRINT("%s: can't find node of key=%u",__FUNCTION__, key);
                continue;
            }
            LIST_FOREACH(pNodeEntry,&pNode->data.treeNodeEntryHead,treeNodeEntry)
            {
                pEntry = pNodeEntry->mibEntry;
                MIB_GetAttrFromBuf(pNode->data.tableIndex,MIB_ATTR_FIRST_INDEX, &id, pEntry->pData,sizeof(id));
                OMCI_PRINT("Entity (%x) Id: 0x%02x",(unsigned int)pEntry, id);
            }

        }
    }
    return GOS_OK;
}

static void omci_GetVlanOperMode(OMCI_VLAN_OPER_MODE_t operMode,char *pStr)
{
    switch(operMode){
    case VLAN_OPER_MODE_FORWARD_ALL:
        sprintf(pStr,"%s","FORWARD ALL");
    break;
    case VLAN_OPER_MODE_FORWARD_UNTAG:
        sprintf(pStr,"%s","FORWARD UNTAG");
    break;
    case VLAN_OPER_MODE_FORWARD_SINGLETAG:
        sprintf(pStr,"%s","FORWARD SINGLE TAG");
    break;
    case VLAN_OPER_MODE_FILTER_INNER_PRI:
        sprintf(pStr,"%s","FILTER PRIORITY TAG");
    break;
    case VLAN_OPER_MODE_FILTER_SINGLETAG:
        sprintf(pStr,"%s","FILTER SINGLELE TAG");
    break;
    case VLAN_OPER_MODE_EXTVLAN:
        sprintf(pStr,"%s","EXTENDED VLAN");
    break;
    case VLAN_OPER_MODE_VLANTAG_OPER:
        sprintf(pStr,"%s","VLAN TAG OPERATION");
    break;
    }
}


static void omci_GetVlanFilterMode(OMCI_VLAN_FILTER_MODE_e filterMode,char *pStr)
{
    if (filterMode & VLAN_FILTER_NO_CARE_TAG)
        sprintf(pStr,"%s","NO CARE TAG");
    if (filterMode & VLAN_FILTER_CARE_TAG)
        sprintf(pStr,"%s","CARE TAG");
    if(filterMode & VLAN_FILTER_NO_TAG)
        sprintf(pStr,"%s","NO TAG");
    if ((filterMode & VLAN_FILTER_VID) && (filterMode & VLAN_FILTER_PRI))
        sprintf(pStr,"%s","FILTER VID+PBIT");
    else if(filterMode & VLAN_FILTER_VID)
        sprintf(pStr,"%s","FILTER VID");
    else if(filterMode & VLAN_FILTER_PRI)
        sprintf(pStr,"%s","FILTER PBIT");
    if(filterMode & VLAN_FILTER_TCI)
        sprintf(pStr,"%s","FILTER TCI");
    if(filterMode & VLAN_FILTER_ETHTYPE)
        sprintf(pStr,"%s","FILTER ETHER TYPE");
}


static void omci_GetVlanActMode(OMCI_VLAN_ACT_MODE_e actMode,char *pStr)
{
    switch(actMode){
    case    VLAN_ACT_NON:
        sprintf(pStr,"%s","NO ACT");
    break;
    case VLAN_ACT_ADD:
        sprintf(pStr,"%s","ADD");
    break;
    case VLAN_ACT_REMOVE:
        sprintf(pStr,"%s","REMOVE");
    break;
    case VLAN_ACT_MODIFY:
        sprintf(pStr,"%s","MODIFY");
    break;
    case VLAN_ACT_TRANSPARENT:
        sprintf(pStr,"%s","TRANSPARENT");
    break;
    }
}


static void omci_GetVidActMode(OMCI_VID_ACT_MODE_e actMode,char *pStr)
{
    switch(actMode){
    case VID_ACT_ASSIGN:
        sprintf(pStr,"%s","ASSIGN");
    break;
    case VID_ACT_COPY_INNER:
        sprintf(pStr,"%s","COPY FROM INNER VID");
    break;
    case VID_ACT_COPY_OUTER:
        sprintf(pStr,"%s","COPY FROM OUTER VID");
    break;
    case VID_ACT_TRANSPARENT:
        sprintf(pStr,"%s","TRANSPARENT");
    break;
    }
}

static void omci_GetPriActMode(OMCI_PRI_ACT_MODE_e actMode,char *pStr)
{
    switch(actMode){
    case PRI_ACT_ASSIGN:
        sprintf(pStr,"%s","ASSIGN");
    break;
    case PRI_ACT_COPY_INNER:
        sprintf(pStr,"%s","COPY FROM INNER PBIT");
    break;
    case PRI_ACT_COPY_OUTER:
        sprintf(pStr,"%s","COPY FROM OUTER PBIT");
    break;
    case PRI_ACT_TRANSPARENT:
        sprintf(pStr,"%s","TRANSPARENT");
    break;
    case PRI_ACT_FROM_DSCP:
        sprintf(pStr,"%s","FROM DSCP");
    break;
    }
}

static void omci_GetDirMode(PON_GEMPORT_DIRECTION dir,char *pStr)
{
    switch(dir){
    case PON_GEMPORT_DIRECTION_US:
        sprintf(pStr,"%s","Upstream");
    break;
    case PON_GEMPORT_DIRECTION_DS:
        sprintf(pStr,"%s","Downstream");
    break;
    case PON_GEMPORT_DIRECTION_BI:
        sprintf(pStr,"%s","Both");
    break;
    }
}

static void omci_GetEtherType(OMCI_VLAN_FILTER_ts *pVlanFilter, char *pStr)
{

    if(pVlanFilter->filterCtagMode & VLAN_FILTER_ETHTYPE)
    {
        switch (pVlanFilter->etherType)
        {
            case ETHTYPE_FILTER_NO_CARE:
                sprintf(pStr,"%s","NO CARE");
                break;
            case ETHTYPE_FILTER_IP:
                sprintf(pStr,"%s","Filter 0x0800");
                break;
            case ETHTYPE_FILTER_PPPOE:
                sprintf(pStr,"%s","Filter 0x8863");
                break;
            case ETHTYPE_FILTER_ARP:
                sprintf(pStr,"%s","Filter 0x0806");
                break;
            case ETHTYPE_FILTER_PPPOE_S:
                sprintf(pStr,"%s","Filter 0x8864");
                break;
            case ETHTYPE_FILTER_IPV6:
                sprintf(pStr,"%s","Filter 0x86dd");
                break;
        }
    }
    else
    {
        sprintf(pStr,"%s","No Filter");
    }
}

static char *covert_out_tpid(unsigned int tpid_id)
{
    switch (tpid_id)
    {
        case TPID_8100:
            return STR(TPID_8100);
        case TPID_88A8:
            return STR(TPID_88A8);
        case TPID_0800:
            return STR(TPID_0800);
        default:
            return STR(NA);
    }
}

static char *covert_tpid(int op, unsigned int tpid_id)
{
    if(op == TPID_ACT)
    {
        switch(tpid_id)
        {
            case EVTOCD_TBL_TREATMENT_TPID_COPY_FROM_INNER:
                return STR(EVTOCD_TBL_TREATMENT_TPID_COPY_FROM_INNER);
            case EVTOCD_TBL_TREATMENT_TPID_COPY_FROM_OUTER:
                return STR(EVTOCD_TBL_TREATMENT_TPID_COPY_FROM_OUTER);
            case EVTOCD_TBL_TREATMENT_TPID_OUTPUT_DEI_CP_INNER:
                return STR(EVTOCD_TBL_TREATMENT_TPID_OUTPUT_DEI_CP_INNER);
            case EVTOCD_TBL_TREATMENT_TPID_OUTPUT_DEI_CP_OUTER:
                return STR(EVTOCD_TBL_TREATMENT_TPID_OUTPUT_DEI_CP_OUTER);
            case EVTOCD_TBL_TREATMENT_TPID_8100:
                return STR(EVTOCD_TBL_TREATMENT_TPID_8100);
            case EVTOCD_TBL_TREATMENT_TPID_OUTPUT_TPID_DEI_0:
                return STR(EVTOCD_TBL_TREATMENT_TPID_OUTPUT_TPID_DEI_0);
            case EVTOCD_TBL_TREATMENT_TPID_OUTPUT_TPID_DEI_1:
                return STR(EVTOCD_TBL_TREATMENT_TPID_OUTPUT_TPID_DEI_1);
            default:
                return STR(NA);
        }
    }
    else
    {
        switch(tpid_id)
        {
            case EVTOCD_TBL_FILTER_TPID_DO_NOT_FILTER:
                return STR(EVTOCD_TBL_FILTER_TPID_DO_NOT_FILTER);
            case EVTOCD_TBL_FILTER_TPID_8100:
                return STR(EVTOCD_TBL_FILTER_TPID_8100);
            case EVTOCD_TBL_FILTER_TPID_INPUT_TPID:
                return STR(EVTOCD_TBL_FILTER_TPID_INPUT_TPID);
            case EVTOCD_TBL_FILTER_TPID_INPUT_TPID_DEI_0:
                return STR(EVTOCD_TBL_FILTER_TPID_INPUT_TPID_DEI_0);
            case EVTOCD_TBL_FILTER_TPID_INPUT_TPID_DEI_1:
                return STR(EVTOCD_TBL_FILTER_TPID_INPUT_TPID_DEI_1);
            default:
                return STR(NA);
        }
    }
}

static GOS_ERROR_CODE MIB_TreeConnRuleDump(omci_vlan_rule_t *pRule)
{
    OMCI_VLAN_OPER_ts *pVlanOper;
    OMCI_VLAN_FILTER_ts *pVlanFilter;
    OMCI_VLAN_ts *pVlan;
    OMCI_VLAN_ACT_ts *pAct;
    OMCI_VLAN_OUT_ts *pOut;
    char str[128]="";

    if(pRule->ingress == 0 && pRule->outgress == 0) return GOS_OK;

    pVlanOper = &pRule->vlanRule;
    pVlanFilter = &pVlanOper->filterRule;
    OMCI_PRINT("==================================================");
    omci_GetDirMode(pRule->dir,str);
    OMCI_PRINT("direction: %s",str);
    if(pRule->dir==PON_GEMPORT_DIRECTION_US || pRule->dir==PON_GEMPORT_DIRECTION_BI)
    {
        OMCI_PRINT("ingress UNI / VEIP / IPHOST: 0x%X",pRule->ingress);
        OMCI_PRINT("engress GemPort: %d",pRule->outgress);
    }else
    {
        OMCI_PRINT("ingress GemPort: %d", pRule->outgress);
        OMCI_PRINT("engress UNI / VEIP / IPHOST: 0x%X", pRule->ingress);
    }
    OMCI_PRINT("servId: %d",pRule->servId);

    omci_GetVlanOperMode(pVlanOper->filterMode,str);
    OMCI_PRINT("filterMode: %s",str);
    omci_GetVlanFilterMode(pVlanFilter->filterStagMode,str);
    OMCI_PRINT("filter S-TAG Mode: %s",str);
    pVlan = &pVlanFilter->filterSTag;
    OMCI_PRINT("(PRI,TPID,VID): (%d,%s,%d)",pVlan->pri, covert_tpid(TPID_FILTER, pVlan->tpid), pVlan->vid);
    omci_GetVlanFilterMode(pVlanFilter->filterCtagMode,str);
    OMCI_PRINT("filter C-TAG Mode: %s",str);
    pVlan = &pVlanFilter->filterCTag;
    OMCI_PRINT("(PRI,TPID,VID): (%d,%s,%d)",pVlan->pri, covert_tpid(TPID_FILTER, pVlan->tpid), pVlan->vid);
    omci_GetEtherType(pVlanFilter, str);
    OMCI_PRINT("filter EtherType: %s",str);
    pAct = &pVlanOper->sTagAct;
    omci_GetVlanActMode(pAct->vlanAct,str);
    OMCI_PRINT("S-TAG Act: %s",str);
    omci_GetVidActMode(pAct->vidAct,str);
    OMCI_PRINT("S-TAG VID Act: %s",str);
    omci_GetPriActMode(pAct->priAct,str);
    OMCI_PRINT("S-TAG PRI Act: %s",str);
    pVlan = &pAct->assignVlan;
    OMCI_PRINT("(PRI,TPID,VID): (%d,%s,%d)",pVlan->pri, covert_tpid(TPID_ACT, pVlan->tpid), pVlan->vid);
    pAct = &pVlanOper->cTagAct;
    omci_GetVlanActMode(pAct->vlanAct,str);
    OMCI_PRINT("C-TAG Act: %s",str);
    omci_GetVidActMode(pAct->vidAct,str);
    OMCI_PRINT("C-TAG VID Act: %s",str);
    omci_GetPriActMode(pAct->priAct,str);
    OMCI_PRINT("C-TAG PRI Act: %s",str);
    pVlan = &pAct->assignVlan;
    OMCI_PRINT("(PRI,TPID,VID): (%d,%s,%d)",pVlan->pri, covert_tpid(TPID_ACT, pVlan->tpid), pVlan->vid);
    pOut = &pVlanOper->outStyle;
    OMCI_PRINT("Out-Style isDefaultRule: %u", pOut->isDefaultRule);
    OMCI_PRINT("Out-Style dsMode: %u", pOut->dsMode);
    OMCI_PRINT("Out-Style dsTagOperMode: %u", pOut->dsTagOperMode);
    OMCI_PRINT("Out-Style outTagNum: %u", pOut->outTagNum);
    //the below is the treatment output tpid result for ME 171 table entries.
    OMCI_PRINT("Out-Style outTpid: %s", covert_out_tpid(pOut->tpid));
    pVlan = &pOut->outVlan;
    OMCI_PRINT("(PRI,TPID,VID): (%d,%s,%d)",pVlan->pri, covert_tpid(TPID_ACT, pVlan->tpid), pVlan->vid);
    OMCI_PRINT("==================================================");
    return GOS_OK;
}


static GOS_ERROR_CODE mib_treeConnDump(MIB_TREE_CONN_T *pConn)
{
    int i;
    omci_vlan_rule_t *pRule;

    if(!pConn)
        return GOS_FAIL;

    OMCI_PRINT("\n\nTraffic Mode: %d",pConn->traffMode);
    OMCI_PRINT("ANI Ptr: 0x%02x",(unsigned int)pConn->pAniPort);
    if (pConn->pUniPort)
    {
        OMCI_PRINT("UNI Ptr: 0x%02x",(unsigned int)pConn->pUniPort);
    }
    else
    {
        OMCI_PRINT("UNI Ptr: 0x00");
    }
    OMCI_PRINT("ETH Ptr: 0x%02x",(unsigned int)pConn->pEthUni);
    OMCI_PRINT("802 Ptr: 0x%02x",(unsigned int)pConn->p8021Map);
    OMCI_PRINT("UNI VlanFilter Ptr: 0x%02x",(unsigned int)pConn->pUniVlanTagFilter);
    OMCI_PRINT("ANI VlanFilter Ptr: 0x%02x",(unsigned int)pConn->pAniVlanTagFilter);
    OMCI_PRINT("UNI VlanTagOper Ptr: 0x%02x",(unsigned int)pConn->pUniVlanTagOpCfg);
    OMCI_PRINT("ANI VlanTagOper Ptr: 0x%02x",(unsigned int)pConn->pAniVlanTagOpCfg);
    OMCI_PRINT("McastGem Ptr: 0x%02x",(unsigned int)pConn->pMcastGemIwTp);
    OMCI_PRINT("UNI ExtVlan Ptr: 0x%02x",(unsigned int)pConn->pUniExtVlanCfg);
    OMCI_PRINT("ANI ExtVlan Ptr: 0x%02x",(unsigned int)pConn->pAniExtVlanCfg);
    OMCI_PRINT("Rule Mode: %s",pConn->traffMode==OMCI_TRAF_MODE_FLOW_BASE ? "FLOW BASE" : "802.1P BASE");
    OMCI_PRINT("==================================================");

    if(pConn->traffMode == OMCI_TRAF_MODE_FLOW_BASE)
    {
        OMCI_PRINT("GemIwTp Ptr: 0x%02x",(unsigned int)pConn->pGemIwTp[0]);
        OMCI_PRINT("GemPortCtp Ptr: 0x%02x",(unsigned int)pConn->pGemPortCtp[0]);
        LIST_FOREACH(pRule,&pConn->ruleHead[0],entries)
        {
            MIB_TreeConnRuleDump(pRule);
        }
    }
    else
    {
        for(i=0;i<8;i++)
        {
            if(pConn->pGemIwTp[i]){

                OMCI_PRINT("For Priority Bit %d",i);
                OMCI_PRINT("GemIwTp Ptr: 0x%02x",(unsigned int)pConn->pGemIwTp[i]);
                OMCI_PRINT("GemPortCtp Ptr: 0x%02x",(unsigned int)pConn->pGemPortCtp[i]);
                LIST_FOREACH(pRule,&pConn->ruleHead[i],entries)
                {
                    MIB_TreeConnRuleDump(pRule);
                }
            }
        }
    }

    return GOS_OK;
}


GOS_ERROR_CODE MIB_TreeConnDump(void)
{
    MIB_TREE_CONN_T *pEntry;

    for (pEntry = LIST_FIRST(&connsInCfg); pEntry != NULL; pEntry = LIST_NEXT(pEntry,entries))
    {
        mib_treeConnDump(pEntry);
    }

    return GOS_OK;
}

void omci_set_tpid(void)
{
    omci_vlan_rule_t* pEntry;
    unsigned idx, max;
    MIB_TREE_CONN_T* pConn  = NULL;

    omci_wrapper_setSvlanTpid(0, 0x88A8);

    for (pConn = LIST_FIRST(&connsInCfg); pConn != NULL; pConn = LIST_NEXT(pConn,entries))
    {
        max = (pConn->traffMode == OMCI_TRAF_MODE_FLOW_BASE ?  1 : 8);
        for (idx = 0; idx < max; idx++)
        {
            if (!pConn->pGemPortCtp[idx])
                continue;
            LIST_FOREACH(pEntry, &pConn->ruleHead[idx], entries)
            {
                if (PON_GEMPORT_DIRECTION_DS == pEntry->dir)
                    continue;

                if ((VLAN_OPER_MODE_EXTVLAN == pEntry->vlanRule.filterMode) &&
                    (2 <= pEntry->vlanRule.outStyle.outTagNum) &&
                    (TPID_8100 == pEntry->vlanRule.outStyle.tpid))
                {
                    omci_wrapper_setSvlanTpid(0, 0x8100);
                    return;
                }

                if (TPID_9100 == pEntry->vlanRule.outStyle.tpid)
                {
                    omci_wrapper_setSvlanTpid(0, 0x9100);
                    return;
                }
            }
        }
    }
    return;
}


static GOS_ERROR_CODE MIB_TreeConnResetAni(MIB_TREE_CONN_T *pConn)
{
    int i;
    for (i = 0; i < 8; i++)
    {
        pConn->pGemIwTp[i]      = NULL;
        pConn->pGemPortCtp[i]   = NULL;
    }
    pConn->pMcastGemIwTp        = NULL;
    pConn->pAniVlanTagFilter    = NULL;
    pConn->pAniVlanTagOpCfg     = NULL;
    pConn->pAniExtVlanCfg       = NULL;
    return GOS_OK;
}

static GOS_ERROR_CODE MIB_TreeConnDefault(MIB_TREE_CONN_T *pConn)
{
    int i;
    memset(pConn,0,sizeof(MIB_TREE_CONN_T));
    pConn->pAniPort = NULL;
    pConn->pUniPort = NULL;
    pConn->pEthUni  = NULL;
    pConn->p8021Map = NULL;
    pConn->pAniVlanTagFilter = NULL;
    pConn->pUniVlanTagFilter = NULL;
    pConn->pAniVlanTagOpCfg = NULL;
    pConn->pUniVlanTagOpCfg = NULL;
    pConn->pMcastGemIwTp = NULL;
    pConn->pAniExtVlanCfg = NULL;
    pConn->pUniExtVlanCfg = NULL;
    pConn->pIpHost = NULL;
    pConn->state = OMCI_CONN_STATE_COMMON;

    for(i=0;i<8;i++)
    {
        pConn->pGemIwTp[i] = NULL;
        pConn->pGemPortCtp[i] = NULL;
        LIST_INIT(&pConn->ruleHead[i]);
    }

    return GOS_OK;
}



static GOS_ERROR_CODE mib_treeConnSet(MIB_TREE_CONN_T *pConn)
{
    GOS_ERROR_CODE ret=0;
    int i,max;

    if(!pConn)
        return GOS_FAIL;

    if(pConn->traffMode == OMCI_TRAF_MODE_FLOW_BASE)
    {
        max = 1;
    }else
    {
        max = 8;
    }

    for(i=0;i<max;i++)
    {
        if(pConn->pGemPortCtp[i])
        {
            ret = OMCI_GenTrafficRule(pConn,i);
            if(ret != GOS_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Fail: GenTraffRule, ret=%d",ret);
                return GOS_FAIL;
            }
        }

    }


    return GOS_OK;
}



static GOS_ERROR_CODE mib_treeConnDel(MIB_TREE_CONN_T *pConn)
{
    GOS_ERROR_CODE ret;
    int i, max = 8;
    omci_vlan_rule_t *pEntry = NULL;

    if(!pConn) return GOS_OK;

    for(i = 0; i < max; i++)
    {
        /* fix that change to flow base from 1P based */
        if(pConn->pGemIwTp[i] || i == 0)
        {
            pEntry = LIST_FIRST(&pConn->ruleHead[i]);
            while(pEntry != NULL)
            {
                if((ret = omci_wrapper_deactiveBdgConn(pEntry->servId, pEntry->ingress))!=GOS_OK)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Fail: DeactiveBdgConn, ret=%d", ret);
                    return GOS_FAIL;
                }

                LIST_REMOVE(pEntry, entries);
                free(pEntry);

                pEntry = LIST_FIRST(&pConn->ruleHead[i]);
            }
            LIST_INIT(&pConn->ruleHead[i]);
        }

        if (FAL_FAIL == feature_api(FEATURE_API_BDP_00000004_DEL_CONN, pConn, i, pEntry))
            return GOS_FAIL;

    }
    return ret;
}

#if 0
static GOS_ERROR_CODE MIB_TreeConnDel(MIB_TREE_T *pTree)
{

    MIB_TREE_CONN_T *pEntry;

    pEntry = LIST_FIRST(&pTree->conns);
    while(pEntry!=NULL)
    {
        mib_treeConnDel(pEntry);
        LIST_REMOVE(pEntry,entries);
        free(pEntry);
        pEntry = LIST_FIRST(&pTree->conns);
    }
    LIST_INIT(&pTree->conns);

    return GOS_OK;
}
#endif

static MIB_TREE_T* mib_avlTreeCreate(MIB_TABLE_MACBRISERVPROF_T *pBridge)
{
    MIB_TREE_T* pTree;

    /*check is tree exist or not*/
    pTree = MIB_AvlTreeSearchByKey(NULL,pBridge->EntityID,AVL_KEY_MACBRISERVPROF);

    if(pTree!=NULL)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: tree is exist!",__FUNCTION__);
        return pTree;
    }

    /*tree not exist, create a new one*/
    forest.treeCount ++;
    /*create a new one avl tree*/
    pTree = (MIB_TREE_T*)malloc(sizeof(MIB_TREE_T));
    pTree->root = NULL;
    LIST_INIT(&pTree->conns);

    /*insert a new data to tree, return new node*/
    if(mib_avlTreeNodeAdd(&pTree->root,AVL_KEY_MACBRISERVPROF,MIB_TABLE_MACBRISERVPROF_INDEX,pBridge)==NULL)
    {
        free(pTree);
        return NULL;
    }
    LIST_INSERT_HEAD(&forest.treeHead,pTree,entries);

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: Add new Tree and root node, [Tree] %x, [Root] %x\n",__FUNCTION__,(unsigned int)pTree,(unsigned int)pTree->root);
    return pTree;
}

MIB_TREE_T* MIB_AvlTreeCreate(MIB_TABLE_MACBRISERVPROF_T *pBridge)
{
    MIB_TREE_T *pTree;

    pthread_mutex_lock(&gOmciTreeCfgMutex);
    pTree = mib_avlTreeCreate(pBridge);
    pthread_mutex_unlock(&gOmciTreeCfgMutex);

    return pTree;
}


static GOS_ERROR_CODE mib_avlTreeNodeEntryRemoveByKey(MIB_NODE_T *pNode, MIB_AVL_KEY_T key, omci_me_instance_t entityId)
{
    MIB_TREE_NODE_ENTRY_T *pNodeEntry;

    pNodeEntry = MIB_AvlTreeEntrySearch(pNode,key,entityId);
    if(pNodeEntry==NULL)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Search entry faild");
        return GOS_OK;
    }
    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "pNodeEntry = %p", pNodeEntry);
    /*remove node from tree*/
    LIST_REMOVE(pNodeEntry,treeNodeEntry);
    free(pNodeEntry);

    return GOS_OK;
}

GOS_ERROR_CODE MIB_AvlTreeNodeEntryRemoveByKey(MIB_NODE_T *pNode, MIB_AVL_KEY_T key, omci_me_instance_t entityId)
{
    GOS_ERROR_CODE retVal;

    pthread_mutex_lock(&gOmciTreeCfgMutex);
    retVal = mib_avlTreeNodeEntryRemoveByKey(pNode, key, entityId);
    pthread_mutex_unlock(&gOmciTreeCfgMutex);

    return retVal;
}


static GOS_ERROR_CODE mib_avlTreeNodeRemove(MIB_NODE_T *pNode)
{
    MIB_TREE_NODE_ENTRY_T *pNodeEntry;
    MIB_TREE_NODE_ENTRY_T *pNodeTmpEntry;

    if(!pNode)
    {
        return GOS_OK;
    }

    mib_avlTreeNodeRemove(pNode->lChild);
    mib_avlTreeNodeRemove(pNode->rChild);

    for(pNodeEntry = LIST_FIRST(&pNode->data.treeNodeEntryHead); pNodeEntry != NULL; )
    {
        pNodeTmpEntry = pNodeEntry;
        pNodeEntry = LIST_NEXT(pNodeEntry,treeNodeEntry);

        LIST_REMOVE(pNodeTmpEntry,treeNodeEntry);
        free(pNodeTmpEntry);
    }
    free(pNode);

    return GOS_OK;
}

GOS_ERROR_CODE MIB_AvlTreeNodeRemove(MIB_NODE_T *pNode)
{
    GOS_ERROR_CODE retVal;

    pthread_mutex_lock(&gOmciTreeCfgMutex);
    retVal = mib_avlTreeNodeRemove(pNode);
    pthread_mutex_unlock(&gOmciTreeCfgMutex);

    return retVal;
}


static GOS_ERROR_CODE mib_avlTreeRemove(MIB_TREE_T *pTree)
{
    MIB_TREE_CONN_T *pConn, *pTempConn;

    /*remove frome forest*/
    LIST_REMOVE(pTree,entries);
    forest.treeCount --;

    /*remove all tree connection*/
    for(pConn = LIST_FIRST(&pTree->conns); pConn!=NULL;)
    {
        pTempConn = pConn;
        pConn = LIST_NEXT(pConn,entries);
        LIST_REMOVE(pTempConn,entries);
        free(pTempConn);
    }

    /*remove all node*/
    mib_avlTreeNodeRemove(pTree->root);

    /* Update */
    MIB_TreeConnUpdate(pTree);

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: remove Tree and nodes",__FUNCTION__);
    return GOS_OK;
}

GOS_ERROR_CODE MIB_AvlTreeRemove(MIB_TREE_T *pTree)
{
    GOS_ERROR_CODE retVal;

    pthread_mutex_lock(&gOmciTreeCfgMutex);
    retVal = mib_avlTreeRemove(pTree);
    pthread_mutex_unlock(&gOmciTreeCfgMutex);

    return retVal;
}

static GOS_ERROR_CODE mib_avlTreeRemoveAll(void)
{
    MIB_TREE_T *pTree;

    for(pTree = LIST_FIRST(&forest.treeHead); pTree!=NULL; pTree = LIST_NEXT(pTree,entries))
    {
        mib_avlTreeRemove(pTree);
    }
    LIST_INIT(&forest.treeHead);
    forest.treeCount = 0;
    return GOS_OK;
}

GOS_ERROR_CODE MIB_AvlTreeRemoveAll(void)
{
    GOS_ERROR_CODE retVal;

    pthread_mutex_lock(&gOmciTreeCfgMutex);
    retVal = mib_avlTreeRemoveAll();
    pthread_mutex_unlock(&gOmciTreeCfgMutex);

    return retVal;
}

GOS_ERROR_CODE omci_GetUniEndByUniPort(MIB_TREE_T *pTree, MIB_TABLE_MACBRIPORTCFGDATA_T *p)
{
    MIB_TABLE_MACBRIPORTCFGDATA_T mbpcd;
    int key = -1;

    if (!p ||!pTree)
        return GOS_FAIL;

    memset(&mbpcd, 0, sizeof(MIB_TABLE_MACBRIPORTCFGDATA_T));
    mbpcd.EntityID = p->EntityID;

    if (GOS_OK != MIB_Get(MIB_TABLE_MACBRIPORTCFGDATA_INDEX, &mbpcd, sizeof(MIB_TABLE_MACBRIPORTCFGDATA_T)))
        return GOS_FAIL;

    switch (mbpcd.TPType)
    {
        case MBPCD_TP_TYPE_PPTP_ETH_UNI:
            key = AVL_KEY_PPTPUNI;
            break;
        case MBPCD_TP_TYPE_VEIP:
            key = AVL_KEY_VEIP;
            break;
        case MBPCD_TP_TYPE_IP_HOST_IPV6_HOST:
            key = AVL_KEY_IPHOST;
            break;
        default:
            return GOS_FAIL;
    }

    if (!MIB_AvlTreeEntrySearch(pTree->root, key, mbpcd.TPPointer))
        return GOS_FAIL;

    return GOS_OK;
}


static GOS_ERROR_CODE MIB_TreeRootConnUpdate(MIB_TREE_T *pTree,MIB_TREE_CONN_T *pConn)
{
    BOOL isExist = FALSE;
    MIB_TREE_CONN_T *pConnEntry, *ptr;
    int i;

    /* update state of connection without uniPort to DEL */
    LIST_FOREACH(pConnEntry, &pTree->conns, entries)
    {
        if (pConnEntry->pAniPort == pConn->pAniPort &&
            (GOS_FAIL == omci_GetUniEndByUniPort(pTree, pConnEntry->pUniPort)))
        {
            pConnEntry->state = OMCI_CONN_STATE_DEL;
        }
    }

    /*insert connection to list, check exist first*/
    LIST_FOREACH(pConnEntry,&pTree->conns,entries)
    {
        if(pConnEntry->pAniPort == pConn->pAniPort && pConnEntry->pUniPort == pConn->pUniPort)
        {
            isExist = TRUE;
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: is connection exist, update it only",__FUNCTION__);
            break;
        }
    }
    /*if exist, update it, else create a new one*/
    if(isExist)
    {
        pConnEntry->state = OMCI_CONN_STATE_UPDATE;
        pConnEntry->pEthUni = pConn->pEthUni;
        for(i=0;i<8;i++)
        {
            pConnEntry->pGemPortCtp[i] = pConn->pGemPortCtp[i];
            pConnEntry->pGemIwTp[i] = pConn->pGemIwTp[i];
        }
        pConnEntry->pMcastGemIwTp = pConn->pMcastGemIwTp;
        pConnEntry->pUniVlanTagFilter = pConn->pUniVlanTagFilter;
        pConnEntry->pAniVlanTagFilter = pConn->pAniVlanTagFilter;
        pConnEntry->pUniVlanTagOpCfg = pConn->pUniVlanTagOpCfg;
        pConnEntry->pAniVlanTagOpCfg = pConn->pAniVlanTagOpCfg;
        pConnEntry->pUniExtVlanCfg = pConn->pUniExtVlanCfg;
        pConnEntry->pAniExtVlanCfg = pConn->pAniExtVlanCfg;
        pConnEntry->traffMode = pConn->traffMode;

    }else
    {
        pConnEntry = (MIB_TREE_CONN_T*)malloc(sizeof(MIB_TREE_CONN_T));
        memset(pConnEntry,0,sizeof(MIB_TREE_CONN_T));
        memcpy(pConnEntry,pConn,sizeof(MIB_TREE_CONN_T));
        pConnEntry->state = OMCI_CONN_STATE_NEW;
        LIST_FOREACH(ptr, &pTree->conns, entries)
        {
            if (NULL == LIST_NEXT(ptr, entries))
                break;
        }
        if (ptr)
            LIST_INSERT_AFTER(ptr, pConnEntry, entries);
        else
            LIST_INSERT_HEAD(&pTree->conns, pConnEntry, entries);
    }

    return GOS_OK;
}

static void MIB_TreeRootConnStateUpdate(MIB_TREE_T *pTree, MIB_TREE_DATA_T *pAniData)
{
    MIB_TREE_CONN_T *pConnEntry = NULL;
    MIB_TREE_NODE_ENTRY_T *pAniDataEntry = NULL;
    MIB_ENTRY_T *pAniEntry = NULL;
    BOOL isExistB = FALSE;
    LIST_FOREACH(pConnEntry, &pTree->conns, entries)
    {
        isExistB = FALSE;
        LIST_FOREACH(pAniDataEntry, &pAniData->treeNodeEntryHead, treeNodeEntry)
        {
            pAniEntry = pAniDataEntry->mibEntry;
            if(pConnEntry->pAniPort->EntityID ==
                ((MIB_TABLE_MACBRIPORTCFGDATA_T*)pAniEntry->pData)->EntityID)
            {
                isExistB = TRUE;
                break;
            }
        }
        if(!isExistB)
        {
            pConnEntry->state = OMCI_CONN_STATE_DEL;
        }

        // TBD: ignore
        if (pConnEntry->pIpHost && pConnEntry->pMcastGemIwTp)
        {
            pConnEntry->state = OMCI_CONN_STATE_COMMON;
        }
    }
}

static GOS_ERROR_CODE MIB_TreeRootConnCheck(MIB_TREE_T *pTree)
{
    MIB_NODE_T  *pUniNode,*pAniNode;
    MIB_TREE_NODE_ENTRY_T *pAniDataEntry,*pUniDataEntry;
    MIB_ENTRY_T *pUniEntry,*pAniEntry;
    MIB_TREE_DATA_T *pUniData,*pAniData;
    int aniOk = GOS_FAIL, uniOk = GOS_FAIL;
    MIB_TABLE_MACBRIPORTCFGDATA_T *pUniPort,*pAniPort;
    MIB_AVL_KEY_T key;
    MIB_TREE_CONN_T conn;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Start %s...",__FUNCTION__);

    /*get ani side MacBridgePort*/
    pAniNode = MIB_AvlTreeSearch(pTree->root,AVL_KEY_MACBRIPORT_ANI);

    if (!pAniNode)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s: can't find MacBridgePortData on ANI side", __FUNCTION__);
        return GOS_FAIL;
    }
    pAniData = &pAniNode->data;

    /*get uni side MacBridgePort*/
    pUniNode = MIB_AvlTreeSearch(pTree->root,AVL_KEY_MACBRIPORT_UNI);

    if (!pUniNode)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s: can't find MacBridgePortData UNI on side", __FUNCTION__);

        MIB_TreeConnDefault(&conn);

        uniOk = GOS_OK;

        LIST_FOREACH(pAniDataEntry,&pAniData->treeNodeEntryHead, treeNodeEntry)
        {
            MIB_TreeConnResetAni(&conn);
            pAniEntry = pAniDataEntry->mibEntry;
            pAniPort = (MIB_TABLE_MACBRIPORTCFGDATA_T*) pAniEntry->pData;

            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "ANI Entity: 0x%02x", pAniPort->EntityID);

            key = AVL_KEY_MACBRIPORT_ANI;

            if (OMCI_MeOperConnCheck(MIB_TABLE_MACBRIPORTCFGDATA_INDEX, pTree, &conn, pAniPort->EntityID, key) != GOS_OK)
                continue;

            aniOk = GOS_OK;
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s: connect to 0x%04x,ANI OK", __FUNCTION__, pAniPort->EntityID);

            /*update connections*/
            MIB_TreeRootConnUpdate(pTree, &conn);
        }

        MIB_TreeRootConnStateUpdate(pTree, pAniData);
        goto finish;
    }

    pUniData = &pUniNode->data;

    /*search each connections included uni+ani*/
    LIST_FOREACH(pUniDataEntry,&pUniData->treeNodeEntryHead, treeNodeEntry)
    {
        MIB_TreeConnDefault(&conn);

        pUniEntry = pUniDataEntry->mibEntry;
        pUniPort = (MIB_TABLE_MACBRIPORTCFGDATA_T*) pUniEntry->pData;
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"UNI Entity: 0x%02x",pUniPort->EntityID);
        key = AVL_KEY_MACBRIPORT_UNI;
        if(OMCI_MeOperConnCheck(MIB_TABLE_MACBRIPORTCFGDATA_INDEX,pTree,&conn,pUniPort->EntityID,key)!=GOS_OK)
        {
            continue;
        }
        if(FAL_ERR_NOT_REGISTER == feature_api_is_registered(FEATURE_API_RDP_00000004))
        {
            if (pUniPort->TPType == MBPCD_TP_TYPE_PPTP_ETH_UNI)
            {
                if (GOS_OK != OMCI_MeOperConnCheck(MIB_TABLE_ETHUNI_INDEX, pTree, &conn, pUniPort->TPPointer, AVL_KEY_PPTPUNI))
                    continue;
            }

            if (pUniPort->TPType == MBPCD_TP_TYPE_VEIP)
            {
                if (GOS_OK != OMCI_MeOperConnCheck(MIB_TABLE_VEIP_INDEX, pTree, &conn, pUniPort->TPPointer, AVL_KEY_VEIP))
                    continue;
            }
        }
        uniOk = GOS_OK;

        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: connect to 0x%04x,UNI OK",__FUNCTION__,pUniPort->EntityID);

        LIST_FOREACH(pAniDataEntry,&pAniData->treeNodeEntryHead, treeNodeEntry)
        {
            MIB_TreeConnResetAni(&conn);
            pAniEntry = pAniDataEntry->mibEntry;
            pAniPort = (MIB_TABLE_MACBRIPORTCFGDATA_T*) pAniEntry->pData;
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"ANI Entity: 0x%02x",pAniPort->EntityID);
            key = AVL_KEY_MACBRIPORT_ANI;
            if(OMCI_MeOperConnCheck(MIB_TABLE_MACBRIPORTCFGDATA_INDEX,pTree,&conn,pAniPort->EntityID,key)!=GOS_OK)
            {
                continue;
            }

            aniOk = GOS_OK;
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: connect to 0x%04x,ANI OK",__FUNCTION__,pAniPort->EntityID);

            /*update connections*/
            MIB_TreeRootConnUpdate(pTree,&conn);
        }

        MIB_TreeRootConnStateUpdate(pTree, pAniData);
    }

finish:
    if(uniOk!=GOS_OK || aniOk!=GOS_OK ){

        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: UNI & ANI check faile!",__FUNCTION__);
        return GOS_FAIL;
    }

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: check connection ok!",__FUNCTION__);

    return GOS_OK;

}

MIB_TREE_T *MIB_TreeFirstGet(void)
{
    MIB_TREE_T *pTreeEntry = NULL;

    pTreeEntry = LIST_FIRST(&forest.treeHead);

    return pTreeEntry;
}

static void mib_tree_connInCfgUpdate(MIB_TREE_T* pTree)
{
    MIB_TREE_CONN_T *pEntry, *pInEntry, *pTmpEntry, *pConnEntry, *ptr = NULL;
    int i;

    /* Walk all connections */
    for (pEntry = LIST_FIRST(&pTree->conns); pEntry != NULL;)
    {
        if (pEntry->state == OMCI_CONN_STATE_DEL)
        {
            /* If the state of connection is DEL, search this connection in connsInCfg and mark it as DEL */
            for (pInEntry = LIST_FIRST(&connsInCfg); pInEntry != NULL; pInEntry = LIST_NEXT(pInEntry,entries))
            {
                if( (pInEntry->pAniPort == pEntry->pAniPort) && (pInEntry->pUniPort == pEntry->pUniPort) )
                {
                    pInEntry->state = OMCI_CONN_STATE_DEL;
                }
            }

            /* Remove connection in forest databse */
            pTmpEntry = pEntry;
            pEntry = LIST_NEXT(pEntry,entries);
            LIST_REMOVE(pTmpEntry,entries);
            free(pTmpEntry);
        }
        else if(pEntry->state == OMCI_CONN_STATE_UPDATE)
        {
            /* If the state of connection is UPDATE, search this connection in connsInCfg and mark it as UPDATE and update connection as parameter */
            for (pInEntry = LIST_FIRST(&connsInCfg); pInEntry != NULL; pInEntry = LIST_NEXT(pInEntry,entries))
            {
                if( (pInEntry->pAniPort == pEntry->pAniPort) && (pInEntry->pUniPort == pEntry->pUniPort) )
                {
                    pInEntry->state = OMCI_CONN_STATE_UPDATE;

                    /* For update connection, update its parameter */
                    pInEntry->pEthUni = pEntry->pEthUni;
                    for(i=0;i<8;i++)
                    {
                        pInEntry->pGemPortCtp[i] = pEntry->pGemPortCtp[i];
                        pInEntry->pGemIwTp[i] = pEntry->pGemIwTp[i];
                    }
                    pInEntry->pMcastGemIwTp = pEntry->pMcastGemIwTp;
                    pInEntry->pUniVlanTagFilter = pEntry->pUniVlanTagFilter;
                    pInEntry->pAniVlanTagFilter = pEntry->pAniVlanTagFilter;
                    pInEntry->pUniVlanTagOpCfg = pEntry->pUniVlanTagOpCfg;
                    pInEntry->pAniVlanTagOpCfg = pEntry->pAniVlanTagOpCfg;
                    pInEntry->pUniExtVlanCfg = pEntry->pUniExtVlanCfg;
                    pInEntry->pAniExtVlanCfg = pEntry->pAniExtVlanCfg;
                    pInEntry->traffMode = pEntry->traffMode;
                }
            }

            pEntry = LIST_NEXT(pEntry,entries);
        }
        else if (pEntry->state == OMCI_CONN_STATE_NEW)
        {
            pConnEntry = (MIB_TREE_CONN_T*)malloc(sizeof(MIB_TREE_CONN_T));
            memcpy(pConnEntry, pEntry, sizeof(MIB_TREE_CONN_T));

            if (ptr == NULL)
                LIST_INSERT_HEAD(&connsInCfg, pConnEntry, entries);
            else
                LIST_INSERT_AFTER(ptr, pConnEntry, entries);

            /* Point to last connection */
            ptr = pConnEntry;
            pEntry = LIST_NEXT(pEntry,entries);
        }
        else if (pEntry->state == OMCI_CONN_STATE_COMMON)
        {
            pEntry = LIST_NEXT(pEntry,entries);
        }
        else
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Unknown Connection State!!");
            pEntry = LIST_NEXT(pEntry,entries);
        }
    }
}

BOOL omci_find_vlan_info_by_ip_host(UINT16 id, UINT16 *pTci)
{
    MIB_TREE_T* pTree       = NULL;
    MIB_TREE_CONN_T* pConn  = NULL;

    if (!(pTree = MIB_AvlTreeSearchByKey(NULL, id, AVL_KEY_IPHOST)))
        return FALSE;

    for (pConn = LIST_FIRST(&connsInCfg); pConn != NULL; pConn = LIST_NEXT(pConn,entries))
    {
        omci_vlan_rule_t* pEntry;
        unsigned idx, max;

        max = (pConn->traffMode == OMCI_TRAF_MODE_FLOW_BASE ?  1 : 8);
        for (idx = 0; idx < max; idx++)
        {
            if (!pConn->pGemPortCtp[idx])
                continue;
            LIST_FOREACH(pEntry, &pConn->ruleHead[idx], entries)
            {
                if (pEntry->dir == PON_GEMPORT_DIRECTION_DS)
                    continue;

                if (pEntry->ingress != id)
                    continue;

                *pTci = ((((pEntry->vlanRule.outStyle.outVlan.pri) & 0x7) << 13) | ((pEntry->vlanRule.outStyle.outVlan.vid) & 0xFFF));
                return TRUE;
            }
        }
    }
    return FALSE;
}

void omci_update_if_vlan(UINT16 *pIf_id)
{
    UINT16                                  id;
    MIB_TABLE_IP_HOST_CFG_DATA_T            mibIpHost;
    MIB_TABLE_IP_HOST_CFG_DATA_T*           pIpHost = NULL;
    MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_T   mibExtIpHost;
    mgmt_cfg_msg_t                          mgmtInfo;

    if (!gInfo.dmMode)
        return;

    for (id = 0; id < TXC_IPHOST_NUM; id++)
    {
        BOOL unlock = TRUE;
        if_channel_mode_t  mode = IF_CHANNEL_MODE_IPOE;
        UINT16              tci = 0xFFFF;

        if (pIf_id && id != *pIf_id)
            continue;

        mibIpHost.EntityID      = id;
        if (GOS_OK != MIB_Get(MIB_TABLE_IP_HOST_CFG_DATA_INDEX, &mibIpHost, sizeof(MIB_TABLE_IP_HOST_CFG_DATA_T)))
        {
            continue;
        }
        if(mibIpHost.IpOptions == IP_HOST_CFG_DATA_IP_OPTIONS_DISABLE_OPTIONS &&
            (mibIpHost.IpAddress == 0 && mibIpHost.Mask == 0 && mibIpHost.Gateway == 0 && mibIpHost.PrimaryDns == 0 && mibIpHost.SecondaryDns == 0))
        {
            continue;
        }

        if (!omci_find_vlan_info_by_ip_host(id, &tci))
            continue;

        mibExtIpHost.EntityId      = id;
        if (GOS_OK == (MIB_Get(MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_INDEX, &mibExtIpHost, sizeof(MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_T))))
            mode = IF_CHANNEL_MODE_PPPOE;

        // set if tci by tr142 api
        memset(&mgmtInfo, 0, sizeof(mgmt_cfg_msg_t));

        mgmtInfo.cfg.if_entry.if_is_ipv6_B = FALSE;
        mgmtInfo.cfg.if_entry.if_is_DHCP_B = (mibIpHost.IpOptions &
                                            IP_HOST_CFG_DATA_IP_OPTIONS_ENABLE_DHCP) ? TRUE : FALSE;
        mgmtInfo.cfg.if_entry.if_is_ip_stack_B = (mibIpHost.IpOptions &
                                                IP_HOST_CFG_DATA_IP_OPTIONS_ENABLE_IP_STACK) ? TRUE : FALSE;

        if(!mgmtInfo.cfg.if_entry.if_is_DHCP_B)
        {
            memcpy(&mgmtInfo.cfg.if_entry.ip_addr.ipv4_addr,
                &(mibIpHost.IpAddress), sizeof(omci_ipv4_addr_t));
            memcpy(&mgmtInfo.cfg.if_entry.mask_addr,
                &(mibIpHost.Mask), sizeof(omci_ipv4_addr_t));
            memcpy(&mgmtInfo.cfg.if_entry.gateway_addr,
                &(mibIpHost.Gateway), sizeof(omci_ipv4_addr_t));
            memcpy(&mgmtInfo.cfg.if_entry.primary_dns_addr,
                &(mibIpHost.PrimaryDns), sizeof(omci_ipv4_addr_t));
            memcpy(&mgmtInfo.cfg.if_entry.second_dns_addr,
                &(mibIpHost.SecondaryDns), sizeof(omci_ipv4_addr_t));
        }

        mgmtInfo.cfg.if_entry.if_id = id;
        mgmtInfo.op_id = OP_SET_IF;
        mgmtInfo.cfg.if_entry.if_tci = tci;
        mgmtInfo.cfg.if_entry.wan_type = mode;

        if(GOS_OK == omci_check_iphost_relation_by_service(IF_SERVICE_SIP, &pIpHost, &unlock) &&
            pIpHost->EntityID == id)
        {
            mgmtInfo.cfg.if_entry.if_service_type |= IF_SERVICE_SIP;
        }
        if(GOS_OK == omci_check_iphost_relation_by_service(IF_SERVICE_TR69, &pIpHost, &unlock) &&
            pIpHost->EntityID == id)
        {
            mgmtInfo.cfg.if_entry.if_service_type |= IF_SERVICE_TR69;
        }

        if(FAL_OK != feature_api(FEATURE_API_RDP_00000010_UPDATE_IF_SIP, pIf_id, &id, &mgmtInfo))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s(): %d Follow standard behavior ", __FUNCTION__, __LINE__);
        }

        if (FAL_OK != feature_api(FEATURE_API_RDP_00000008_UPDATE_IF_TR69, pIf_id, &id, &mgmtInfo))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s(): %d Follow standard behavior ", __FUNCTION__, __LINE__);
        }


        if (IF_CHANNEL_MODE_PPPOE == mode)
        {
            //only for internet
            mgmtInfo.cfg.pppoe.related_if_id = id;
            mgmtInfo.cfg.pppoe.nat_enabled = TRUE; //TBD
            mgmtInfo.cfg.pppoe.auth_method = mibExtIpHost.Mode;
            mgmtInfo.cfg.pppoe.conn_type = mibExtIpHost.Connect;
            mgmtInfo.cfg.pppoe.release_time = mibExtIpHost.ReleaseTime;
            strncpy(mgmtInfo.cfg.pppoe.username, mibExtIpHost.User, MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_STRING_PART_LEN);
            strncpy(mgmtInfo.cfg.pppoe.password, mibExtIpHost.Password, MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_STRING_PART_LEN);
        }

        feature_api(FEATURE_API_L3SVC_MGMT_CFG_SET, &mgmtInfo, sizeof(mgmt_cfg_msg_t));

    }

    return;
}


static void mib_tree_allTreeConnUpdate(UINT16   classID,
                                        UINT16  instanceID,
                                        UINT32  privData)
{
    BOOL isNewCmplt = TRUE;
    MIB_TREE_T* pTree;
    MIB_TREE_CONN_T *pEntry, *pTmpEntry;
    /* delete timer */
    omci_timer_delete_by_id(classID, instanceID);
    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "All trees update timer expired");

    for (pEntry = LIST_FIRST(&connsInCfg); pEntry != NULL;)
    {
        /* Mark all connections as DEL, if there no such connection in current tree/forest, this connection will be deleted. */
        pEntry->state = OMCI_CONN_STATE_DEL;
        pEntry = LIST_NEXT(pEntry,entries);
    }

    /* Start Process when current treeCount > 0 */
    if(forest.treeCount > 0)
    {
        pthread_mutex_lock(&gOmciTreeCfgMutex);

        LIST_FOREACH(pTree,&forest.treeHead,entries)
        {
            isNewCmplt = ((GOS_FAIL == MIB_TreeRootConnCheck(pTree)) ?  FALSE : TRUE);

            /*start to handler new traffic*/
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"New Connection is %s", isNewCmplt == TRUE ? "complete" : "not complete");

            if (FAL_OK != feature_api(FEATURE_API_BDP_00000004_UPDATE_CONN, pTree))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_WARN, "TBD ");
            }

            /*start to handle connections*/
            if (isNewCmplt == TRUE)
            {
                mib_tree_connInCfgUpdate(pTree);
            }
        }

        pthread_mutex_unlock(&gOmciTreeCfgMutex);

    }

    /* Apply all connections to Kernel */
    for (pEntry = LIST_FIRST(&connsInCfg); pEntry != NULL;)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: state %d", __FUNCTION__, pEntry->state);
        switch(pEntry->state)
        {
            case OMCI_CONN_STATE_DEL:
                mib_treeConnDel(pEntry);
                pTmpEntry = pEntry;
                pEntry = LIST_NEXT(pEntry,entries);
                LIST_REMOVE(pTmpEntry,entries);
                free(pTmpEntry);
            break;
            default:
                pEntry = LIST_NEXT(pEntry,entries);
            break;
        }
    }
    /* Apply all connections to Kernel */
    for (pEntry = LIST_FIRST(&connsInCfg); pEntry != NULL;)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: state %d", __FUNCTION__, pEntry->state);
        switch(pEntry->state)
        {
            case OMCI_CONN_STATE_NEW:
                mib_treeConnSet(pEntry);
                pEntry = LIST_NEXT(pEntry,entries);
            break;
            case OMCI_CONN_STATE_COMMON:
                pEntry = LIST_NEXT(pEntry,entries);
            break;
            case OMCI_CONN_STATE_UPDATE:
                mib_treeConnDel(pEntry);
                mib_treeConnSet(pEntry);
                pEntry = LIST_NEXT(pEntry,entries);
            break;
            default:
                pEntry = LIST_NEXT(pEntry,entries);
            break;
        }
    }

    omci_update_conn_if_mod_tpid();

    omci_update_if_vlan(NULL);
}

GOS_ERROR_CODE MIB_TreeConnUpdate(MIB_TREE_T *pTree)
{
    UINT8             sendBuf[sizeof(WORK_MSG_HDR_T)];
    GOS_ERROR_CODE    ret;
    WORK_MSG_HDR_T    *pHdr;

    if(pTree == NULL)
        return GOS_FAIL;

    memset(sendBuf, 0x00, sizeof(WORK_MSG_HDR_T));
    pHdr = (WORK_MSG_HDR_T *)sendBuf;
    pHdr->msgType = 1;

    if ((ret= msgsnd(pOperTask->msgQId, (WORK_MSG_HDR_T *)sendBuf, sizeof(WORK_MSG_HDR_T) - (2 * sizeof(long)), 0)) == -1)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Send Tree Connection Configuration Task failed!");
    }

    return GOS_OK;
}

static GOS_ERROR_CODE mib_treeOperCfgTask(void  *pData)
{
    ssize_t             retSize;
    WORK_MSG_HDR_T      *pMsg;
    UINT8               msgBuff[sizeof(WORK_MSG_HDR_T)];
    size_t              msgSize = sizeof(WORK_MSG_HDR_T) - (2 * sizeof(long));
    GOS_ERROR_CODE  ret = GOS_OK;
    omci_timer_entry_t  *pEntry;

    pMsg = (WORK_MSG_HDR_T*)msgBuff;

    omci_timer_setSignalMask(OMCI_TASK_TREE_ACCEPT_SIG_MASK);

    while(NULL == pOperTask)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Wait tree cfg init done ...");
        OMCI_TaskDelay(100);
    }

    while (1)
    {
        retSize = msgrcv(pOperTask->msgQId, (WORK_MSG_HDR_T*)pMsg, msgSize, 0, 0);

        if(-1 == retSize)
        {
            if(EINTR == errno)
            {
                /* A signal is caught, just continue */
            }
            else
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "[OMCI:%s:%d] msgq recv failed %d\n", __FILE__, __LINE__, errno);
            }
            continue;
        }

        pEntry = omci_timer_search(OMCI_TIMER_RESERVED_CLASS_ID,
                                    OMCI_TIMER_APPLY_TREE_UPDATE_ID);

        if (pEntry)
        {
            // restart timer
            ret = omci_timer_restart(OMCI_TIMER_RESERVED_CLASS_ID,
                                    OMCI_TIMER_APPLY_TREE_UPDATE_ID,
                                    2, 250000000, FALSE);
            if (GOS_OK != ret)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Restart timer for tree update fail");
                return GOS_FAIL;
            }
        }
        else
        {
            // create timer for msg expire monitoring
            ret = omci_timer_create(OMCI_TIMER_RESERVED_CLASS_ID,
                                    OMCI_TIMER_APPLY_TREE_UPDATE_ID,
    	                        	2, 250000000, FALSE, 0, mib_tree_allTreeConnUpdate,OMCI_TIMER_SIG_MIB_TREE);
            if (GOS_OK != ret)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Create timer for tree update fail");
                return GOS_FAIL;
            }
        }
    }

    return GOS_OK;
}

GOS_ERROR_CODE MIB_TreeCfgTaskInit(void)
{
    int taskId ;
    taskId = OMCI_SpawnTask("Tree CONN configuration",
                            mib_treeOperCfgTask,
                            NULL,
                            OMCI_TASK_PRI_MIB,
                            TRUE);
    if(taskId == OMCI_TASK_ID_INVALID)
    {
        return GOS_FAIL;
    }
    pOperTask = OMCI_GetTaskInfo(taskId);

    if(pOperTask==NULL)
    {
        return GOS_FAIL;
    }

    return GOS_OK;
}

void omci_generate_vlan_info(void)
{
    FILE *pVlanInfoFile = NULL;
    MIB_TREE_CONN_T *pEntry = NULL;
    MIB_TREE_T *pTree = NULL;
    omci_vlan_rule_t *pRule = NULL;
    OMCI_VLAN_ts *pVlan = NULL;
    UINT32 i, max, count = 0;

    pVlanInfoFile = fopen("/tmp/omci_vlan_info", "w");

    if(NULL == pVlanInfoFile)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s@%d open omci_vlan_info failed ", __FUNCTION__, __LINE__);
        return;
    }

    LIST_FOREACH(pTree, &forest.treeHead, entries)
    {
        for (pEntry = LIST_FIRST(&pTree->conns); pEntry != NULL; pEntry=LIST_NEXT(pEntry,entries))
        {
            max = ((pEntry->traffMode == OMCI_TRAF_MODE_FLOW_BASE) ? 1 : 8);
            for (i = 0; i < max; i++)
            {
                LIST_FOREACH(pRule, &pEntry->ruleHead[i], entries)
                {
                    if (pRule->dir == PON_GEMPORT_DIRECTION_US ||
                        pRule->dir == PON_GEMPORT_DIRECTION_BI)
                    {
                        count++;
                        switch (pRule->vlanRule.filterMode)
                        {
                            case VLAN_OPER_MODE_FORWARD_ALL:
                                fprintf(pVlanInfoFile, "[%d]: %s\t| FWD Any\n",
                                    count, (NULL == pEntry->p8021Map ? "N:1" : "unknown"));
                                break;
                            case VLAN_OPER_MODE_FORWARD_UNTAG:
                                fprintf(pVlanInfoFile, "[%d]: %s\t| FWD Untag\n",
                                    count, (NULL == pEntry->p8021Map ? "VLAN(P)" : "VLAN+802MAP(MP)"));
                                break;
                            case VLAN_OPER_MODE_FORWARD_SINGLETAG:
                                fprintf(pVlanInfoFile, "[%d]: %s\t| FWD 1 Tag\n",
                                    count, (NULL == pEntry->p8021Map ? "VLAN(P)" : "VLAN+802MAP(MP)"));
                                break;
                            case VLAN_OPER_MODE_FILTER_INNER_PRI:
                                pVlan = &(pRule->vlanRule.filterRule.filterCTag);
                                fprintf(pVlanInfoFile, "[%d]: %s\t| FWD (%u,0)\n",
                                    count, (NULL == pEntry->p8021Map ? "VLAN(P)" : "VLAN+802MAP(MP)"), pVlan->pri);
                                break;
                            case VLAN_OPER_MODE_FILTER_SINGLETAG:
                                pVlan = &(pRule->vlanRule.filterRule.filterCTag);
                                fprintf(pVlanInfoFile, "[%d]: %s\t| FWD (%u,%u)\n",
                                    count, (NULL == pEntry->p8021Map ? "VLAN(P)" : "VLAN+802MAP(MP)"), pVlan->pri, pVlan->vid);
                                break;
                            case VLAN_OPER_MODE_VLANTAG_OPER:
                            case VLAN_OPER_MODE_EXTVLAN:
                                if ((VLAN_ACT_NON == pRule->vlanRule.cTagAct.vlanAct ||
                                    VLAN_ACT_TRANSPARENT == pRule->vlanRule.cTagAct.vlanAct) &&
                                    (VLAN_ACT_NON == pRule->vlanRule.sTagAct.vlanAct ||
                                    VLAN_ACT_TRANSPARENT == pRule->vlanRule.sTagAct.vlanAct))
                                {
                                    pVlan = &(pRule->vlanRule.filterRule.filterCTag);
                                    fprintf(pVlanInfoFile, "[%d]: %s\t| FWD (%u,%u)\n",
                                        count, (NULL == pEntry->p8021Map ? "VLAN(P)" : "VLAN+802MAP(MP)"), pVlan->pri, pVlan->vid);
                                }
                                else
                                {
                                    fprintf(pVlanInfoFile, "[%d]: %s\t| FWD (%u,%u)\n",
                                        count, (NULL == pEntry->p8021Map ? "VLAN(P)" : "VLAN+802MAP(MP)"),
                                        pRule->vlanRule.outStyle.outVlan.pri, pRule->vlanRule.outStyle.outVlan.vid);
                                }
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
        }
    }
    fflush(pVlanInfoFile);
    fclose(pVlanInfoFile);
    return;
}

static GOS_ERROR_CODE omci_generate_node(FILE *pDotFile, MIB_TABLE_INDEX tableId)
{
    MIB_TABLE_T         *pTable = NULL;
    MIB_ENTRY_T         *pMibEntry;
    omci_me_instance_t  instId;
    if (!(pTable = mib_GetTablePtr(tableId)))
        return GOS_FAIL;

    LIST_FOREACH(pMibEntry, &pTable->entryHead, entries)
    {
        instId = *((omci_me_instance_t *)pMibEntry->pData);
        if (MIB_TABLE_MACBRISERVPROF_INDEX == tableId)
            fprintf(pDotFile, "\t\t %s_%#x [style=\"rounded,filled\" fillcolor=greenyellow root=true]\n", MIB_GetTableShortName(tableId), instId);
        else
            fprintf(pDotFile, "\t\t %s_%#x\n", MIB_GetTableShortName(tableId), instId);
    }
    return GOS_OK;
}

static void omci_generate_dot_node_onu(FILE *pDotFile)
{
    MIB_AVL_KEY_T key;
    MIB_TABLE_INDEX     tableId;

    fprintf(pDotFile, "\t {\n");
    fprintf(pDotFile, "\t\t node [shape=box fixedsize=true width=2]\n");
    for (key = AVL_KEY_PPTPUNI; key <= AVL_KEY_TCONT; key++)
    {
        if (AVL_KEY_PPTPUNI == key || AVL_KEY_VEIP == key) {
            tableId = (AVL_KEY_PPTPUNI == key ? MIB_TABLE_ETHUNI_INDEX: MIB_TABLE_VEIP_INDEX);
            //each onu node and label it
            if (GOS_OK != omci_generate_node(pDotFile, tableId)) {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s@%d generate node failed ", __FUNCTION__, __LINE__);
            }
        }
    }
    fprintf(pDotFile, "\t }\n");
    return;
}

static void omci_generate_dot_node_olt(FILE *pDotFile)
{
    MIB_AVL_KEY_T key;
    MIB_TABLE_INDEX     tableId;

    fprintf(pDotFile, "\t {\n");
    fprintf(pDotFile, "\t\t node [shape=box style=rounded fixedsize=true width=2]\n");
    for (key = AVL_KEY_PPTPUNI; key <= AVL_KEY_TCONT; key++)
    {
        if (AVL_KEY_PPTPUNI == key || AVL_KEY_VEIP == key ||
            AVL_KEY_PRIQ == key || AVL_KEY_TCONT == key)
        {
            continue;
        }
        switch (key)
        {
            case AVL_KEY_EXTVLAN_UNI:
            case AVL_KEY_EXTVLAN_ANI:
                tableId = MIB_TABLE_EXTVLANTAGOPERCFGDATA_INDEX;
                break;
            case AVL_KEY_VLANTAGFILTER_UNI:
            case AVL_KEY_VLANTAGFILTER_ANI:
                tableId = MIB_TABLE_VLANTAGFILTERDATA_INDEX;
                break;
            case AVL_KEY_VLANTAGOPCFG_UNI:
            case AVL_KEY_VLANTAGOPCFG_ANI:
                tableId = MIB_TABLE_VLANTAGOPCFGDATA_INDEX;
                break;
            case AVL_KEY_MACBRIPORT_UNI:
            case AVL_KEY_MACBRIPORT_ANI:
                tableId = MIB_TABLE_MACBRIPORTCFGDATA_INDEX;
                break;
            case AVL_KEY_MACBRISERVPROF:
                tableId = MIB_TABLE_MACBRISERVPROF_INDEX;
                break;
            case AVL_KEY_MAP8021PSERVPROF:
                tableId = MIB_TABLE_MAP8021PSERVPROF_INDEX;
                break;
            case AVL_KEY_GEMIWTP:
                tableId = MIB_TABLE_GEMIWTP_INDEX;
                break;
            case AVL_KEY_MULTIGEMIWTP:
                tableId = MIB_TABLE_MULTIGEMIWTP_INDEX;
                break;
            case AVL_KEY_GEMPORTCTP:
                tableId = MIB_TABLE_GEMPORTCTP_INDEX;
                break;
            default:
                OMCI_LOG(OMCI_LOG_LEVEL_WARN, "%s@%d not support ", __FUNCTION__, __LINE__);
            continue;
        }

        //each olt node and label it
        if (GOS_OK != omci_generate_node(pDotFile, tableId))
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s@%d generate node failed ", __FUNCTION__, __LINE__);
    }
    //mscd and mop node
    if (GOS_OK != omci_generate_node(pDotFile, MIB_TABLE_MCASTSUBCONFINFO_INDEX))
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s@%d generate node failed ", __FUNCTION__, __LINE__);
    if (GOS_OK != omci_generate_node(pDotFile, MIB_TABLE_MCASTOPERPROF_INDEX))
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s@%d generate node failed ", __FUNCTION__, __LINE__);

    fprintf(pDotFile, "\t }\n");
    return;
}

static GOS_ERROR_CODE
omci_get_tableId_by_association_type(UINT8 type)
{
    switch (type)
    {
        case EVTOCD_ASSOC_TYPE_MAC_BRIDGE_PORT:
            return MIB_TABLE_MACBRIPORTCFGDATA_INDEX;
        case EVTOCD_ASSOC_TYPE_IEEE_8021P_MAPPER:
            return MIB_TABLE_MAP8021PSERVPROF_INDEX;
        case EVTOCD_ASSOC_TYPE_PPTP_ETH_UNI:
            return MIB_TABLE_ETHUNI_INDEX;
        case EVTOCD_ASSOC_TYPE_IP_HOST_IPV6_HOST:
            return MIB_TABLE_IP_HOST_CFG_DATA_INDEX;
        case EVTOCD_ASSOC_TYPE_GEM_IWTP:
            return MIB_TABLE_GEMIWTP_INDEX;
        case EVTOCD_ASSOC_TYPE_MCAST_GEM_IWTP:
            return MIB_TABLE_MULTIGEMIWTP_INDEX;
        case EVTOCD_ASSOC_TYPE_VEIP:
            return MIB_TABLE_VEIP_INDEX;
        default:
            return MIB_TABLE_UNKNOWN_INDEX;
    }
}

static GOS_ERROR_CODE
omci_get_tableId_by_tp_type(UINT8 type)
{
    switch (type)
    {
        case MBPCD_TP_TYPE_PPTP_ETH_UNI:
            return MIB_TABLE_ETHUNI_INDEX;
        case MBPCD_TP_TYPE_IEEE_8021P_MAPPER:
            return MIB_TABLE_MAP8021PSERVPROF_INDEX;
        case MBPCD_TP_TYPE_IP_HOST_IPV6_HOST:
            return MIB_TABLE_IP_HOST_CFG_DATA_INDEX;
        case MBPCD_TP_TYPE_GEM_IWTP:
            return MIB_TABLE_GEMIWTP_INDEX;
        case MBPCD_TP_TYPE_MCAST_GEM_IWTP:
            return MIB_TABLE_MULTIGEMIWTP_INDEX;
        case MBPCD_TP_TYPE_VEIP:
            return MIB_TABLE_VEIP_INDEX;
        default:
            return MIB_TABLE_UNKNOWN_INDEX;
    }
}

static GOS_ERROR_CODE
omci_get_tableId_by_vlan_opcfg_type(UINT8 type)
{
    switch (type)
    {
        case VTOCD_ASSOC_TYPE_DEFAULT:
        case VTOCD_ASSOC_TYPE_PPTP_ETH_UNI:
            return MIB_TABLE_ETHUNI_INDEX;
        case VTOCD_ASSOC_TYPE_IP_HOST_IPV6_HOST:
            return MIB_TABLE_IP_HOST_CFG_DATA_INDEX;
        case VTOCD_ASSOC_TYPE_IEEE_8021P_MAPPER:
            return MIB_TABLE_MAP8021PSERVPROF_INDEX;
        case VTOCD_ASSOC_TYPE_GEM_IWTP:
            return MIB_TABLE_GEMIWTP_INDEX;
        case VTOCD_ASSOC_TYPE_MCAST_GEM_IWTP:
            return MIB_TABLE_MULTIGEMIWTP_INDEX;
        case VTOCD_ASSOC_TYPE_VEIP:
            return MIB_TABLE_VEIP_INDEX;
        default:
            return MIB_TABLE_UNKNOWN_INDEX;
    }
}

static void omci_generate_extvlan_relationship(FILE *pDotFile, MIB_NODE_T *pNode)
{
    MIB_TREE_NODE_ENTRY_T               *pNodeEntry = NULL;
    MIB_ENTRY_T                         *pEntry = NULL;
    MIB_TABLE_EXTVLANTAGOPERCFGDATA_T   *pExtVlan = NULL;
    MIB_TABLE_INDEX                     tableId;

    LIST_FOREACH(pNodeEntry, &pNode->data.treeNodeEntryHead, treeNodeEntry)
    {
        pEntry = pNodeEntry->mibEntry;
        pExtVlan = (MIB_TABLE_EXTVLANTAGOPERCFGDATA_T *)pEntry->pData;
        if (MIB_TABLE_UNKNOWN_INDEX == (tableId =
            omci_get_tableId_by_association_type(pExtVlan->AssociationType)))
        {
            continue;
        }
        fprintf(pDotFile, "\t %s_%#x -> %s_%#x\n",
            MIB_GetTableShortName(pNode->data.tableIndex),
            pExtVlan->EntityId,
            MIB_GetTableShortName(tableId),
            pExtVlan->AssociatedMePoint);

    }
    return;
}

static void omci_generate_vlanopcfg_relationship(FILE *pDotFile, MIB_NODE_T *pNode)
{
    MIB_TREE_NODE_ENTRY_T           *pNodeEntry = NULL;
    MIB_ENTRY_T                     *pEntry = NULL;
    MIB_TABLE_VLANTAGOPCFGDATA_T    *pVlanOpCfg = NULL;
   // UINT16                          id;
    MIB_TABLE_INDEX                 tableId;

    LIST_FOREACH(pNodeEntry, &pNode->data.treeNodeEntryHead, treeNodeEntry)
    {
        pEntry = pNodeEntry->mibEntry;
        pVlanOpCfg = (MIB_TABLE_VLANTAGOPCFGDATA_T *)pEntry->pData;
        if (MIB_TABLE_UNKNOWN_INDEX == (tableId =
            omci_get_tableId_by_vlan_opcfg_type(pVlanOpCfg->Type)))
        {
            continue;
        }

     //   if (VTOCD_ASSOC_TYPE_DEFAULT == pVlanOpCfg->Type)
     //       id = pVlanOpCfg->EntityID;
     //   else
      //      id = pVlanOpCfg->Pointer;

        fprintf(pDotFile, "\t %s_%#x -> %s_%#x\n",
            MIB_GetTableShortName(pNode->data.tableIndex),
            pVlanOpCfg->EntityID,
            MIB_GetTableShortName(tableId),
            pVlanOpCfg->Pointer);
    }
    return;
}

static void omci_generate_macbp_relationship(FILE *pDotFile, MIB_NODE_T *pNode)
{
    MIB_TREE_NODE_ENTRY_T           *pNodeEntry = NULL;
    MIB_ENTRY_T                     *pEntry = NULL;
    MIB_TABLE_MACBRIPORTCFGDATA_T   *pPort = NULL;
    MIB_TABLE_MCASTSUBCONFINFO_T    mcastConfInfo;
    mopTableEntry_t                 *pMopEntry = NULL;
    MIB_TABLE_VLANTAGFILTERDATA_T   vlanTagFilter;
    MIB_TABLE_INDEX                 tableId;

    LIST_FOREACH(pNodeEntry, &pNode->data.treeNodeEntryHead, treeNodeEntry)
    {
        pEntry = pNodeEntry->mibEntry;
        pPort = (MIB_TABLE_MACBRIPORTCFGDATA_T *)pEntry->pData;

        if (MIB_TABLE_UNKNOWN_INDEX == (tableId =
            omci_get_tableId_by_tp_type(pPort->TPType)))
        {
            continue;
        }

        fprintf(pDotFile, "\t %s_%#x -> %s_%#x\n",
            MIB_GetTableShortName(pNode->data.tableIndex),
            pPort->EntityID,
            MIB_GetTableShortName(tableId),
            pPort->TPPointer);

        vlanTagFilter.EntityID = pPort->EntityID;
        if(GOS_OK == MIB_Get(MIB_TABLE_VLANTAGFILTERDATA_INDEX, &vlanTagFilter,
            sizeof(MIB_TABLE_VLANTAGFILTERDATA_T)))
        {
            fprintf(pDotFile, "\t %s_%#x -> %s_%#x [dir=none style=dashed]\n",
                MIB_GetTableShortName(MIB_TABLE_MACBRIPORTCFGDATA_INDEX),
                pPort->EntityID,
                MIB_GetTableShortName(MIB_TABLE_VLANTAGFILTERDATA_INDEX),
                vlanTagFilter.EntityID);
        }

        mcastConfInfo.EntityId = pPort->EntityID;
        if(GOS_OK == MIB_Get(MIB_TABLE_MCASTSUBCONFINFO_INDEX, &mcastConfInfo,
            sizeof(MIB_TABLE_MCASTSUBCONFINFO_T)))
        {
            fprintf(pDotFile, "\t %s_%#x -> %s_%#x [dir=none style=dashed]\n",
                MIB_GetTableShortName(MIB_TABLE_MACBRIPORTCFGDATA_INDEX),
                pPort->EntityID,
                MIB_GetTableShortName(MIB_TABLE_MCASTSUBCONFINFO_INDEX),
                mcastConfInfo.EntityId);

            LIST_FOREACH(pMopEntry, &mcastConfInfo.MOPhead, entries)
            {
                fprintf(pDotFile, "\t %s_%#x -> %s_%#x\n",
                    MIB_GetTableShortName(MIB_TABLE_MCASTSUBCONFINFO_INDEX),
                    mcastConfInfo.EntityId,
                    MIB_GetTableShortName(MIB_TABLE_MCASTOPERPROF_INDEX),
                    pMopEntry->tableEntry.mcastOperProfPtr);
            }
        }

        fprintf(pDotFile, "\t %s_%#x -> %s_%#x\n",
            MIB_GetTableShortName(pNode->data.tableIndex),
            pPort->EntityID,
            MIB_GetTableShortName(MIB_TABLE_MACBRISERVPROF_INDEX),
            pPort->BridgeIdPtr);
    }
    return;
}

static void omci_generate_1p_relationship(FILE *pDotFile, MIB_NODE_T *pNode)
{
    MIB_TREE_NODE_ENTRY_T *pNodeEntry = NULL;
    MIB_ENTRY_T *pEntry = NULL;
    MIB_TABLE_MAP8021PSERVPROF_T *pMapper = NULL;
    UINT32 i;
    UINT16 *ptr = NULL;
    LIST_FOREACH(pNodeEntry, &pNode->data.treeNodeEntryHead, treeNodeEntry)
    {
        pEntry = pNodeEntry->mibEntry;
        pMapper = (MIB_TABLE_MAP8021PSERVPROF_T *)pEntry->pData;
        if (pMapper->IwTpPtrPbit0 == pMapper->IwTpPtrPbit1 &&
            pMapper->IwTpPtrPbit0 == pMapper->IwTpPtrPbit2 &&
            pMapper->IwTpPtrPbit0 == pMapper->IwTpPtrPbit3 &&
            pMapper->IwTpPtrPbit0 == pMapper->IwTpPtrPbit4 &&
            pMapper->IwTpPtrPbit0 == pMapper->IwTpPtrPbit5 &&
            pMapper->IwTpPtrPbit0 == pMapper->IwTpPtrPbit6 &&
            pMapper->IwTpPtrPbit0 == pMapper->IwTpPtrPbit7)
        {

            fprintf(pDotFile, "\t %s_%#x -> %s_%#x [dir=both label=\" pbit=0...7\"]\n",
                MIB_GetTableShortName(pNode->data.tableIndex),
                pMapper->EntityID,
                MIB_GetTableShortName(MIB_TABLE_GEMIWTP_INDEX),
                pMapper->IwTpPtrPbit0);
        }
        else
        {
            for (i = 0; i < 8 ; i++)
            {
                ptr = &pMapper->IwTpPtrPbit0 + i;
                if (ptr && *ptr != 0xffff)
                {
                    fprintf(pDotFile, "\t %s_%#x -> %s_%#x [dir=both label=\"pbit=%d\"]\n",
                        MIB_GetTableShortName(pNode->data.tableIndex),
                        pMapper->EntityID,
                        MIB_GetTableShortName(MIB_TABLE_GEMIWTP_INDEX),
                        *ptr, i);
                }
            }
        }
    }
    return;
}

static void omci_generate_gemiwtp_relationship(FILE *pDotFile, MIB_NODE_T *pNode)
{
    MIB_TREE_NODE_ENTRY_T *pNodeEntry = NULL;
    MIB_ENTRY_T *pEntry = NULL;
    MIB_TABLE_GEMIWTP_T *pGemIwtp = NULL;
    LIST_FOREACH(pNodeEntry, &pNode->data.treeNodeEntryHead, treeNodeEntry)
    {
        pEntry = pNodeEntry->mibEntry;
        pGemIwtp = (MIB_TABLE_GEMIWTP_T *)pEntry->pData;

        fprintf(pDotFile, "\t %s_%#x -> %s_%#x\n",
            MIB_GetTableShortName(pNode->data.tableIndex),
            pGemIwtp->EntityID,
            MIB_GetTableShortName(MIB_TABLE_GEMPORTCTP_INDEX),
            pGemIwtp->GemCtpPtr);

    }
    return;
}

static void omci_generate_mcgemiwtp_relationship(FILE *pDotFile, MIB_NODE_T *pNode)
{
    MIB_TREE_NODE_ENTRY_T *pNodeEntry = NULL;
    MIB_ENTRY_T *pEntry = NULL;
    MIB_TABLE_MULTIGEMIWTP_T *pMcGemIwtp = NULL;
    LIST_FOREACH(pNodeEntry, &pNode->data.treeNodeEntryHead, treeNodeEntry)
    {
        pEntry = pNodeEntry->mibEntry;
        pMcGemIwtp = (MIB_TABLE_MULTIGEMIWTP_T *)pEntry->pData;

        fprintf(pDotFile, "\t %s_%#x -> %s_%#x\n",
            MIB_GetTableShortName(pNode->data.tableIndex),
            pMcGemIwtp->EntityID,
            MIB_GetTableShortName(MIB_TABLE_GEMPORTCTP_INDEX),
            pMcGemIwtp->GemCtpPtr);

    }
    return;
}

static void omci_generate_gemctp_relationship(FILE *pDotFile, MIB_NODE_T *pNode)
{
    MIB_TREE_NODE_ENTRY_T *pNodeEntry = NULL;
    MIB_ENTRY_T *pEntry = NULL;
    MIB_TABLE_GEMPORTCTP_T *pGemCtp = NULL;

    LIST_FOREACH(pNodeEntry, &pNode->data.treeNodeEntryHead, treeNodeEntry)
    {
        pEntry = pNodeEntry->mibEntry;
        pGemCtp = (MIB_TABLE_GEMPORTCTP_T *)pEntry->pData;
        if (PON_GEMPORT_DIRECTION_DS != pGemCtp->Direction)
        {
            fprintf(pDotFile, "\t {\n");
            fprintf(pDotFile, "\t\t node [shape=box fixedsize=true width=2]\n");
            fprintf(pDotFile, "\t\t %s_%#x\n",
                MIB_GetTableShortName(MIB_TABLE_PRIQ_INDEX),
                pGemCtp->UsTraffMgmtPtr);
            fprintf(pDotFile, "\t\t %s_%#x\n",
                MIB_GetTableShortName(MIB_TABLE_TCONT_INDEX),
                pGemCtp->TcAdapterPtr);
            fprintf(pDotFile, "\t }\n");

            fprintf(pDotFile, "\t %s_%#x -> %s_%#x\n",
                MIB_GetTableShortName(MIB_TABLE_GEMPORTCTP_INDEX),
                pGemCtp->EntityID,
                MIB_GetTableShortName(MIB_TABLE_PRIQ_INDEX),
                pGemCtp->UsTraffMgmtPtr);

            fprintf(pDotFile, "\t %s_%#x -> %s_%#x\n",
                MIB_GetTableShortName(MIB_TABLE_GEMPORTCTP_INDEX),
                pGemCtp->EntityID,
                MIB_GetTableShortName(MIB_TABLE_TCONT_INDEX),
                pGemCtp->TcAdapterPtr);
        }
    }
    return;
}

static void omci_generate_dot_relationship(FILE *pDotFile, MIB_NODE_T *pNode)
{
    if(!pNode)
    {
        return;
    }

    if (pNode->lChild)
        omci_generate_dot_relationship(pDotFile, pNode->lChild);

    switch (pNode->data.key)
    {
        case AVL_KEY_EXTVLAN_UNI:
        case AVL_KEY_EXTVLAN_ANI:
            omci_generate_extvlan_relationship(pDotFile, pNode);
            break;
        case AVL_KEY_VLANTAGOPCFG_UNI:
        case AVL_KEY_VLANTAGOPCFG_ANI:
            omci_generate_vlanopcfg_relationship(pDotFile, pNode);
            break;
        case AVL_KEY_MACBRIPORT_UNI:
        case AVL_KEY_MACBRIPORT_ANI:
            omci_generate_macbp_relationship(pDotFile, pNode);
            break;
        case AVL_KEY_MAP8021PSERVPROF:
            omci_generate_1p_relationship(pDotFile, pNode);
            break;
        case AVL_KEY_GEMIWTP:
            omci_generate_gemiwtp_relationship(pDotFile, pNode);
            break;
        case AVL_KEY_MULTIGEMIWTP:
            omci_generate_mcgemiwtp_relationship(pDotFile, pNode);
            break;
        case AVL_KEY_GEMPORTCTP:
            omci_generate_gemctp_relationship(pDotFile, pNode);
            break;
        default:
            break;
    }

    if (pNode->rChild)
        omci_generate_dot_relationship(pDotFile, pNode->rChild);

    return;
}

void omci_generate_dot_file(void)
{
    FILE *pDotFile = NULL;
    MIB_TREE_T *pTree = NULL;

    pDotFile = fopen("/tmp/omci_dot_file", "w");

    if(NULL == pDotFile)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s@%d open omci_dot_file failed ", __FUNCTION__, __LINE__);
        return;
    }

    fprintf(pDotFile, "digraph G {\n");

    /* data path node for onu self create */
    omci_generate_dot_node_onu(pDotFile);
    /* data path node for olt create */
    omci_generate_dot_node_olt(pDotFile);

    /* relationship */
    LIST_FOREACH(pTree, &forest.treeHead, entries)
    {
        omci_generate_dot_relationship(pDotFile, pTree->root);
    }

    fprintf(pDotFile, "}\n");
    fflush(pDotFile);
    fclose(pDotFile);
    return;
}

GOS_ERROR_CODE omci_mcast_port_reset(UINT16 portId, MIB_TABLE_MCASTSUBCONFINFO_T* pMibMcastSubCfgInfo, UINT16 mcastOperProfId)
{
    BOOL resetB = TRUE;
    UINT32 size;
    UINT8 *pVal = NULL;
    mopTableEntry_t *ptr = NULL;
    //if the port exist other mcastOper inforation, then reset below information
    if(pMibMcastSubCfgInfo &&
        pMibMcastSubCfgInfo->curMopCnt > 0)
    {
        LIST_FOREACH(ptr, &pMibMcastSubCfgInfo->MOPhead, entries)
        {
            if(ptr->tableEntry.mcastOperProfPtr != mcastOperProfId)
                resetB = FALSE;
        }
    }
    else if (NULL == pMibMcastSubCfgInfo)
    {
        resetB = FALSE;
    }
    if(resetB)
    {
        MCAST_WRAPPER(omci_igmp_function_set, mcastOperProfId, portId, UCHAR_MAX);
		MCAST_WRAPPER(omci_igmp_version_set, mcastOperProfId, portId, UCHAR_MAX);
        MCAST_WRAPPER(omci_immediate_leave_set, mcastOperProfId, portId, UCHAR_MAX);
        MCAST_WRAPPER(omci_us_igmp_rate_set, mcastOperProfId, portId, 0);
        MCAST_WRAPPER(omci_robustness_set, portId, 0);
        //private behavior
        size = (mcastOperProfId > 65279 ? 16 : 4);
        pVal = (UINT8 *)malloc(sizeof(UINT8) * size);
        memset(pVal, 0, size);
        MCAST_WRAPPER(omci_querier_ip_addr_set, mcastOperProfId, (void *)pVal, size);
        MCAST_WRAPPER(omci_query_interval_set, mcastOperProfId, 125); //unit: second
        MCAST_WRAPPER(omci_query_max_response_time_set, mcastOperProfId, 10); //unit: second
        MCAST_WRAPPER(omci_last_member_query_interval_set, mcastOperProfId, 1); //unit: second
        free(pVal);
    }
    return GOS_OK;
}

