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
 * Purpose : Definition of ME handler: PPTP Ethernet UNI (11)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: PPTP Ethernet UNI (11)
 */

#include "app_basic.h"
#ifndef OMCI_X86
#include "common/error.h"
#include "rtk/port.h"
#endif

MIB_TABLE_INFO_T gMibEthUniTableInfo;
MIB_ATTR_INFO_T  gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ATTR_NUM];
MIB_TABLE_ETHUNI_T gMibEthUniDefRow;
MIB_TABLE_OPER_T gMibEthUniOper;
UINT32 pptpPtMask = 0;

GOS_ERROR_CODE EthUniConnCheck(MIB_TREE_T *pTree,MIB_TREE_CONN_T *pConn,omci_me_instance_t entity,int parm)
{
    MIB_TREE_NODE_ENTRY_T *pNodeEntry;
    MIB_ENTRY_T *pEntry;
    MIB_TABLE_ETHUNI_T *ethUni;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Start %s...",__FUNCTION__);
    pNodeEntry = MIB_AvlTreeEntrySearch(pTree->root,AVL_KEY_PPTPUNI,entity);

    if(!pNodeEntry)
        return GOS_FAIL;

    pEntry = pNodeEntry->mibEntry;
    ethUni = (MIB_TABLE_ETHUNI_T*)pEntry->pData;
    pConn->pEthUni = ethUni;
    OMCI_MeOperConnCheck(MIB_TABLE_EXTVLANTAGOPERCFGDATA_INDEX,pTree,pConn,ethUni->EntityID,AVL_KEY_PPTPUNI);
    OMCI_MeOperConnCheck(MIB_TABLE_VLANTAGFILTERDATA_INDEX,pTree,pConn,ethUni->EntityID,parm);
    OMCI_MeOperConnCheck(MIB_TABLE_VLANTAGOPCFGDATA_INDEX, pTree, pConn, ethUni->EntityID, AVL_KEY_PPTPUNI);
    return GOS_OK;
}

GOS_ERROR_CODE EthUniAvlTreeAdd(MIB_TREE_T *pTree,UINT16 ethUniId)
{
    MIB_TABLE_ETHUNI_T *pEthUni;

    if(pTree==NULL) return GOS_FAIL;

    pEthUni = (MIB_TABLE_ETHUNI_T*)malloc(sizeof(MIB_TABLE_ETHUNI_T));

    if(!pEthUni)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Malloc EthUni Fail");
        return GOS_FAIL;
    }
    memset(pEthUni,0,sizeof(MIB_TABLE_ETHUNI_T));
    pEthUni->EntityID = ethUniId;
    if(MIB_Get(MIB_TABLE_ETHUNI_INDEX, pEthUni, sizeof(MIB_TABLE_ETHUNI_T))!=GOS_OK)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Get EthUni Fail");
        free(pEthUni);
        return GOS_FAIL;
    }

    if(MIB_AvlTreeNodeAdd(&pTree->root,AVL_KEY_PPTPUNI,MIB_TABLE_ETHUNI_INDEX,pEthUni)== NULL)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Add ETHUNI Node Fail");
        free(pEthUni);
        return GOS_FAIL;
    }
    return GOS_OK;
}

GOS_ERROR_CODE EthUniAvlTreeDel(MIB_TREE_T *pTree, UINT16 ethUniId)
{
    MIB_TABLE_ETHUNI_T  mibPptpEthUni;
    MIB_TABLE_EXTVLANTAGOPERCFGDATA_T   *pExtVlan   = NULL;
    MIB_TREE_NODE_ENTRY_T               *pNodeEntry = NULL;
    MIB_ENTRY_T                         *pEntry     = NULL;
    MIB_NODE_T                          *pNode      = NULL;

    if (NULL == pTree)
        return GOS_FAIL;

    mibPptpEthUni.EntityID = ethUniId;
    if (GOS_OK != MIB_Get(MIB_TABLE_ETHUNI_INDEX, &mibPptpEthUni, sizeof(mibPptpEthUni)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_ETHUNI_INDEX), mibPptpEthUni.EntityID);

        return GOS_FAIL;
    }

    if ((pNode = MIB_AvlTreeSearch(pTree->root, AVL_KEY_EXTVLAN_UNI)))
    {
        LIST_FOREACH(pNodeEntry, &pNode->data.treeNodeEntryHead, treeNodeEntry)
        {
            if (!(pEntry = pNodeEntry->mibEntry))
                continue;

            if (!(pExtVlan = (MIB_TABLE_EXTVLANTAGOPERCFGDATA_T*)pEntry->pData))
                continue;

            if (EVTOCD_ASSOC_TYPE_PPTP_ETH_UNI == pExtVlan->AssociationType &&
                ethUniId == pExtVlan->AssociatedMePoint)
            {
                MIB_AvlTreeNodeEntryRemoveByKey(pTree->root, AVL_KEY_EXTVLAN_UNI, pExtVlan->EntityId);
                break;
            }
        }
    }

    if (GOS_OK != MIB_AvlTreeNodeEntryRemoveByKey(pTree->root, AVL_KEY_PPTPUNI, mibPptpEthUni.EntityID))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Remove AVL node fail in %s: %s, 0x%x, tree 0x%p",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_ETHUNI_INDEX), mibPptpEthUni.EntityID, pTree);

        return GOS_FAIL;
    }

    return GOS_OK;
}

static GOS_ERROR_CODE pptp_eth_uni_link_status_updater(unsigned int         portId,
                                                        MIB_TABLE_ETHUNI_T  *pMibPptpEthUni)
{
    GOS_ERROR_CODE              ret = GOS_OK;
    omci_port_link_status_t     linkStatus;

    if (!pMibPptpEthUni)
        return GOS_ERR_PARAM;

    linkStatus.port = portId;

    ret = omci_wrapper_getPortLinkStatus(&linkStatus);
    if (GOS_OK != ret)
        return GOS_FAIL;

    pMibPptpEthUni->OpState = linkStatus.status ?
        OMCI_ME_ATTR_OP_STATE_ENABLED : OMCI_ME_ATTR_OP_STATE_DISABLED;

    return ret;
}

static GOS_ERROR_CODE pptp_eth_uni_speed_duplex_updater(unsigned int        portId,
                                                        MIB_TABLE_ETHUNI_T  *pMibPptpEthUni)
{
    GOS_ERROR_CODE                      ret = GOS_OK;
    omci_port_speed_duplex_status_t     speedDuplexStatus;

    if (!pMibPptpEthUni)
        return GOS_ERR_PARAM;

    speedDuplexStatus.port = portId;

    ret = omci_wrapper_getPortSpeedDuplexStatus(&speedDuplexStatus);
    if (GOS_OK != ret)
        return GOS_FAIL;

    switch (speedDuplexStatus.speed)
    {
        case OMCI_PORT_SPEED_10M:
            if (OMCI_PORT_FULL_DUPLEX == speedDuplexStatus.duplex)
                pMibPptpEthUni->DuplexInd = PPTP_ETH_UNI_CFG_IND_10BASET_FULL_DUPLEX;
            else
                pMibPptpEthUni->DuplexInd = PPTP_ETH_UNI_CFG_IND_10BASET_HALF_DUPLEX;
            break;

        case OMCI_PORT_SPEED_100M:
            if (OMCI_PORT_FULL_DUPLEX == speedDuplexStatus.duplex)
                pMibPptpEthUni->DuplexInd = PPTP_ETH_UNI_CFG_IND_100BASET_FULL_DUPLEX;
            else
                pMibPptpEthUni->DuplexInd = PPTP_ETH_UNI_CFG_IND_100BASET_HALF_DUPLEX;
            break;

        case OMCI_PORT_SPEED_1000M:
            if (OMCI_PORT_FULL_DUPLEX == speedDuplexStatus.duplex)
                pMibPptpEthUni->DuplexInd = PPTP_ETH_UNI_CFG_IND_GIGABIT_ETHERNET_FULL_DUPLEX;
            else
                pMibPptpEthUni->DuplexInd = PPTP_ETH_UNI_CFG_IND_GIGABIT_ETHERNET_HALF_DUPLEX;
            break;

        default:
            break;
    }

    return ret;
}

GOS_ERROR_CODE pptp_eth_uni_set_auto_detect(unsigned int        portId,
                                            MIB_TABLE_ETHUNI_T  *pMibPptpEthUni)
{
    GOS_ERROR_CODE                  ret = GOS_OK;
    omci_port_auto_nego_ability_t   autoNegoAbility;

    if (!pMibPptpEthUni)
        return GOS_ERR_PARAM;

    autoNegoAbility.port = portId;
    autoNegoAbility.full_10 = FALSE;
    autoNegoAbility.half_10 = FALSE;
    autoNegoAbility.full_100 = FALSE;
    autoNegoAbility.half_100 = FALSE;
    autoNegoAbility.full_1000 = FALSE;
    autoNegoAbility.half_1000 = FALSE;

    switch (pMibPptpEthUni->AutoDectectCfg)
    {
        case PPTP_ETH_UNI_AUTO_DETECT_CFG_10M_FULL_DUPLEX:
            autoNegoAbility.full_10 = TRUE;
            break;
        case PPTP_ETH_UNI_AUTO_DETECT_CFG_100M_FULL_DUPLEX:
            autoNegoAbility.full_100 = TRUE;
            break;
        case PPTP_ETH_UNI_AUTO_DETECT_CFG_1000M_FULL_DUPLEX:
            autoNegoAbility.full_1000 = TRUE;
            break;
        case PPTP_ETH_UNI_AUTO_DETECT_CFG_10M_HALF_DUPLEX:
            autoNegoAbility.half_10 = TRUE;
            break;
        case PPTP_ETH_UNI_AUTO_DETECT_CFG_100M_HALF_DUPLEX:
            autoNegoAbility.half_100 = TRUE;
            break;
        case PPTP_ETH_UNI_AUTO_DETECT_CFG_1000M_HALF_DUPLEX:
            autoNegoAbility.half_1000 = TRUE;
            break;
        case PPTP_ETH_UNI_AUTO_DETECT_CFG_10M_AUTO:
            autoNegoAbility.half_10 = TRUE;
            autoNegoAbility.full_10 = TRUE;
            break;
        case PPTP_ETH_UNI_AUTO_DETECT_CFG_100M_AUTO:
            autoNegoAbility.half_100 = TRUE;
            autoNegoAbility.full_100 = TRUE;
            break;
        case PPTP_ETH_UNI_AUTO_DETECT_CFG_1000M_AUTO:
            autoNegoAbility.half_1000 = TRUE;
            autoNegoAbility.full_1000 = TRUE;
            break;
        case PPTP_ETH_UNI_AUTO_DETECT_CFG_AUTO_FULL_DUPLEX:
            autoNegoAbility.full_10 = TRUE;
            autoNegoAbility.full_100 = TRUE;
            autoNegoAbility.full_1000 = TRUE;
            break;
        case PPTP_ETH_UNI_AUTO_DETECT_CFG_AUTO_HALF_DUPLEX:
            autoNegoAbility.half_10 = TRUE;
            autoNegoAbility.half_100 = TRUE;
            autoNegoAbility.half_1000 = TRUE;
            break;
        case PPTP_ETH_UNI_AUTO_DETECT_CFG_AUTO_AUTO:
            autoNegoAbility.half_10 = TRUE;
            autoNegoAbility.full_10 = TRUE;
            autoNegoAbility.half_100 = TRUE;
            autoNegoAbility.full_100 = TRUE;
            autoNegoAbility.half_1000 = TRUE;
            autoNegoAbility.full_1000 = TRUE;
            break;
        default:
            break;
    }

    ret = omci_wrapper_setPortAutoNegoAbility(&autoNegoAbility);
    if (GOS_OK != ret)
        return GOS_FAIL;

    return GOS_OK;
}

GOS_ERROR_CODE pptp_eth_uni_set_admin_state(unsigned int        portId,
                                            MIB_TABLE_ETHUNI_T  *pMibPptpEthUni)
{
    GOS_ERROR_CODE      ret = GOS_OK;
    omci_port_state_t   portState;
    omci_port_pwr_down_t portPwrDownState;

    if (!pMibPptpEthUni)
        return GOS_ERR_PARAM;

    portState.port = portId;
    portState.state =
        (OMCI_ME_ATTR_ADMIN_STATE_LOCK == pMibPptpEthUni->AdminState) ? FALSE : TRUE;

    ret = omci_wrapper_setPortState(&portState);
    if (GOS_OK != ret)
        return GOS_FAIL;

    portPwrDownState.port  = portId;
    portPwrDownState.state =
        (OMCI_ME_ATTR_ADMIN_STATE_LOCK == pMibPptpEthUni->AdminState) ? TRUE: FALSE;
    ret = omci_wrapper_setPhyPwrDown (&portPwrDownState);
    if (GOS_OK != ret) {
        return (GOS_FAIL);
    }

    return GOS_OK;
}

GOS_ERROR_CODE pptp_eth_uni_set_max_frame_size(unsigned int         portId,
                                                MIB_TABLE_ETHUNI_T  *pMibPptpEthUni)
{
    GOS_ERROR_CODE              ret = GOS_OK;
    omci_port_max_frame_size_t  portMaxFrameSize;

    if (!pMibPptpEthUni)
        return GOS_ERR_PARAM;

    portMaxFrameSize.port = portId;
    portMaxFrameSize.size = pMibPptpEthUni->MaxFrameSize;

    ret = omci_wrapper_setMaxFrameSize(&portMaxFrameSize);
    if (GOS_OK != ret)
        return GOS_FAIL;

    return GOS_OK;
}

GOS_ERROR_CODE pptp_eth_uni_set_loopback(unsigned int       portId,
                                        MIB_TABLE_ETHUNI_T  *pMibPptpEthUni)
{
    GOS_ERROR_CODE          ret = GOS_OK;
    omci_port_loopback_t    portLoopback;

    if (!pMibPptpEthUni)
        return GOS_ERR_PARAM;

    if (PPTP_ETH_UNI_LOOPBACK_CFG_NO_LOOPBACK != pMibPptpEthUni->EthLoopbackCfg &&
            PPTP_ETH_UNI_LOOPBACK_CFG_DS_PHY_LOOPBACK != pMibPptpEthUni->EthLoopbackCfg)
        return GOS_ERR_PARAM;

    portLoopback.port = portId;
    portLoopback.loopback =
        (PPTP_ETH_UNI_LOOPBACK_CFG_NO_LOOPBACK == pMibPptpEthUni->EthLoopbackCfg) ? FALSE : TRUE;

    ret = omci_wrapper_setPhyLoopback(&portLoopback);
    if (GOS_OK != ret)
        return GOS_FAIL;

    return GOS_OK;
}

GOS_ERROR_CODE pptp_eth_uni_set_pause_time(unsigned int       portId,
                                        MIB_TABLE_ETHUNI_T  *pMibPptpEthUni)
{
    GOS_ERROR_CODE          ret = GOS_OK;
    omci_port_pause_ctrl_t    portPauseCtrl;

    if (!pMibPptpEthUni)
        return GOS_ERR_PARAM;

    portPauseCtrl.port = portId;
    portPauseCtrl.pause_time = pMibPptpEthUni->PauseTime;

    ret = omci_wrapper_setPauseControl(&portPauseCtrl);
    if (GOS_OK != ret)
        return GOS_FAIL;

    return GOS_OK;
}

GOS_ERROR_CODE 
pptp_eth_uni_set_briIpInd (
    unsigned int       portId,
    MIB_TABLE_ETHUNI_T  *pMibPptpEthUni)
{
    GOS_ERROR_CODE          ret = GOS_OK;
    char* cmd = malloc ( sizeof( char ) * 100 );

    if (!pMibPptpEthUni || !cmd)
    {
        ret = GOS_ERR_PARAM;
        goto ERROR_BRIIPIND;
    }

    if (pptpPtMask == 0)
    {
        pptpPtMask |= (1 << gInfo.devCapabilities.ponPort);
    }
    
    if (pMibPptpEthUni->BridgedorIPInd == PPTP_ETH_UNI_BRIIPIND_BRIDGED)
    {
        sprintf(cmd, "brctl delif br0 eth0.%u", pMibPptpEthUni->PhysicalPortId + 2);
        pptpPtMask |= ( 1 << portId );
    } 
    else if (pMibPptpEthUni->BridgedorIPInd == PPTP_ETH_UNI_BRIIPIND_ROUTER)
    {
        sprintf(cmd, "brctl addif br0 eth0.%u", pMibPptpEthUni->PhysicalPortId + 2);
        pptpPtMask &= ~( 1 << portId );
    }
    else
    {
        goto ERROR_BRIIPIND;
    }

    OMCI_LOG (
        OMCI_LOG_LEVEL_WARN,
        "%s\n", 
        cmd);
    system ( cmd );
    
    OMCI_LOG (
        OMCI_LOG_LEVEL_WARN,
        "pptpPortMask 0x%x\n", 
        pptpPtMask);

    memset ( cmd, 0x00, sizeof(*cmd));
    sprintf(cmd, "echo %u > /proc/rg/hybrid_pptp_portmask", pptpPtMask);
    system ( cmd );

ERROR_BRIIPIND:
    free( cmd );
    cmd = NULL;
    OMCI_LOG (
        OMCI_LOG_LEVEL_WARN,
        "pMibPptpEthUni or cmd is not existed...%d", ret);
    return GOS_OK;
}

GOS_ERROR_CODE EthUniDrvCfg(void            *pOldRow,
                            void            *pNewRow,
                            MIB_OPERA_TYPE  operationType,
                            MIB_ATTRS_SET   attrSet,
                            UINT32          pri)
{
    GOS_ERROR_CODE      ret = GOS_OK;
    MIB_TABLE_INDEX     tableIndex = MIB_TABLE_ETHUNI_INDEX;
    omci_me_instance_t  instanceID;
    UINT16              portId;
    unig_attr_mgmt_capability_t mngCapUnig = UNIG_MGMT_CAPABILITY_BOTH_OMCI_NON_OMCI;

    // read out the instanceID
    MIB_GetAttrFromBuf(tableIndex,
        MIB_ATTR_FIRST_INDEX, &instanceID, pNewRow, sizeof(omci_me_instance_t));
    //
    // Check manage Capability of UNI-G
    //
    if (omci_mngCapOfUnigGet (instanceID, &mngCapUnig) != GOS_OK)
    {
        OMCI_LOG (OMCI_LOG_LEVEL_WARN, "Failed to get UNI-G for EthUni(0x%x)\n", instanceID);
    }

    // translate me id to switch port id
    if (GOS_OK != pptp_eth_uni_me_id_to_switch_port(instanceID, &portId))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance ID translate error: %s, 0x%x",
            MIB_GetTableName(tableIndex), instanceID);

        return GOS_FAIL;
    }

    switch (operationType)
    {
        case MIB_ADD:
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"PPtP ETH UNI Configure ---- > ADD");
            if (UNIG_MGMT_CAPABILITY_NON_OMCI_ONLY != mngCapUnig)
            {
                // set auto detect and admin state at mib initialization
                if((ret = pptp_eth_uni_set_auto_detect(portId, (MIB_TABLE_ETHUNI_T *)pNewRow)) != GOS_OK)
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set port %d Auto Detect Error: 0x%X", portId, ret);

                if((ret = pptp_eth_uni_set_admin_state(portId, (MIB_TABLE_ETHUNI_T *)pNewRow)) != GOS_OK)
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set port %d Admin State Error: 0x%X", portId, ret);

                // set max frame size
                if((ret = pptp_eth_uni_set_max_frame_size(portId, (MIB_TABLE_ETHUNI_T *)pNewRow)) != GOS_OK)
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set port %d Max Frame Size Error: 0x%X", portId, ret);

                // set loopback
                if((ret = pptp_eth_uni_set_loopback(portId, (MIB_TABLE_ETHUNI_T *)pNewRow)) != GOS_OK)
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set port %d Loopback Error: 0x%X", portId, ret);

                /* set pause time */
                if((ret = pptp_eth_uni_set_pause_time(portId, (MIB_TABLE_ETHUNI_T *)pNewRow)) != GOS_OK)
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set port %d Pause Timer Error: 0x%X", portId, ret);

                if((ret = pptp_eth_uni_set_briIpInd(portId, (MIB_TABLE_ETHUNI_T *)pNewRow)) != GOS_OK)
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set port %d briIpInd Error: 0x%X", portId, ret);

            }
            ret = mib_alarm_table_add(tableIndex, pNewRow);

            break;

        case MIB_DEL:
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"PPtP ETH UNI Configure ---- > DEL");

            ret = mib_alarm_table_del(tableIndex, pOldRow);

            break;

        case MIB_SET:
            if (UNIG_MGMT_CAPABILITY_NON_OMCI_ONLY != mngCapUnig)
            {
                if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ETHUNI_ARC_INDEX))
                {
                    ret = omci_arc_timer_processor(tableIndex,
                        pOldRow, pNewRow, MIB_TABLE_ETHUNI_ARC_INDEX, MIB_TABLE_ETHUNI_ARCINTERVAL_INDEX);
                }

                if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ETHUNI_AUTODECTECTCFG_INDEX))
                {
                    if (((MIB_TABLE_ETHUNI_T *)pNewRow)->AutoDectectCfg !=
                            ((MIB_TABLE_ETHUNI_T *)pOldRow)->AutoDectectCfg)
                        ret = pptp_eth_uni_set_auto_detect(portId, (MIB_TABLE_ETHUNI_T *)pNewRow);
                }

                if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ETHUNI_ADMINSTATE_INDEX))
                {
                    if (((MIB_TABLE_ETHUNI_T *)pNewRow)->AdminState !=
                            ((MIB_TABLE_ETHUNI_T *)pOldRow)->AdminState)
                        ret = pptp_eth_uni_set_admin_state(portId, (MIB_TABLE_ETHUNI_T *)pNewRow);
                }

                if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ETHUNI_MAXFRAMESIZE_INDEX))
                {
                    if (((MIB_TABLE_ETHUNI_T *)pNewRow)->MaxFrameSize !=
                            ((MIB_TABLE_ETHUNI_T *)pOldRow)->MaxFrameSize)
                        ret = pptp_eth_uni_set_max_frame_size(portId, (MIB_TABLE_ETHUNI_T *)pNewRow);
                }

                if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ETHUNI_ETHLOOPBACKCFG_INDEX))
                {
                    if (((MIB_TABLE_ETHUNI_T *)pNewRow)->EthLoopbackCfg !=
                            ((MIB_TABLE_ETHUNI_T *)pOldRow)->EthLoopbackCfg)
                        ret = pptp_eth_uni_set_loopback(portId, (MIB_TABLE_ETHUNI_T *)pNewRow);
                }

                if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ETHUNI_PAUSETIME_INDEX))
                {
                    if (((MIB_TABLE_ETHUNI_T *)pNewRow)->PauseTime !=
                            ((MIB_TABLE_ETHUNI_T *)pOldRow)->PauseTime)
                        ret = pptp_eth_uni_set_pause_time(portId, (MIB_TABLE_ETHUNI_T *)pNewRow);
                }

                if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ETHUNI_PAUSETIME_INDEX))
                {
                    if (((MIB_TABLE_ETHUNI_T *)pNewRow)->PauseTime !=
                            ((MIB_TABLE_ETHUNI_T *)pOldRow)->PauseTime)
                        ret = pptp_eth_uni_set_pause_time(portId, (MIB_TABLE_ETHUNI_T *)pNewRow);
                }

                if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ETHUNI_BRIDGEDORIPIND_INDEX))
                {
                    if (((MIB_TABLE_ETHUNI_T *)pNewRow)->BridgedorIPInd !=
                            ((MIB_TABLE_ETHUNI_T *)pOldRow)->BridgedorIPInd )
                    {
                        ret = pptp_eth_uni_set_briIpInd (portId, (MIB_TABLE_ETHUNI_T *)pNewRow);
                    }
                }
            }
            break;

        default:
            break;
    }

    // only for status update
    {
        MIB_TABLE_ETHUNI_T  mibPptpEthUni;

        mibPptpEthUni.EntityID = instanceID;

        // read out the entry data
        if (GOS_OK == MIB_Get(tableIndex, &mibPptpEthUni, sizeof(mibPptpEthUni)))
        {
            // update link status
            if (GOS_OK == pptp_eth_uni_link_status_updater(portId, &mibPptpEthUni))
            {
                // update speed/duplex if link is up
                if (OMCI_ME_ATTR_OP_STATE_ENABLED == mibPptpEthUni.OpState)
                    pptp_eth_uni_speed_duplex_updater(portId, &mibPptpEthUni);
                else
                {
                    // raise LOS alarm at MIB add
                    if (MIB_ADD == operationType)
                    {
                        mib_alarm_table_t   alarmTable;

                        if (GOS_OK == mib_alarm_table_get(tableIndex, instanceID, &alarmTable))
                        {
                            alarmTable.aBitMask[m_omci_alm_nbr_in_byte(OMCI_ALM_NBR_ETHERNET_LAN_LOS)] |=
                                m_omci_alm_nbr_in_bit(OMCI_ALM_NBR_ETHERNET_LAN_LOS);

                            if (GOS_OK != mib_alarm_table_set(tableIndex, instanceID, &alarmTable))
                            {
                                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set alarm table fail: %s, 0x%x",
                                MIB_GetTableName(tableIndex), instanceID);
                            }
                        }
                    }
                }
            }
            // update phy port id by instance id
            mibPptpEthUni.PhysicalPortId = portId;
            // invoke AVC callback and update attribute value
            MIB_Set(tableIndex, &mibPptpEthUni, sizeof(mibPptpEthUni));
        }
    }

    return ret;
}

static GOS_ERROR_CODE pptp_eth_uni_alarm_handler(MIB_TABLE_INDEX        tableIndex,
                                                    omci_alm_data_t     alarmData,
                                                    omci_me_instance_t  *pInstanceID,
                                                    BOOL                *pIsUpdated)
{
    mib_alarm_table_t   alarmTable;
    BOOL                isSuppressed;
    MIB_ENTRY_T         *pEntry;
    MIB_TABLE_ETHUNI_T  *pMibPptpEthUni;
    omci_me_instance_t  cpInstanceID;
    UINT16              slotId, portId;

    if (!pInstanceID || !pIsUpdated)
        return GOS_ERR_PARAM;

    *pIsUpdated = FALSE;

    // extract instanceID from alarm detail
    // portId should be the switch port number
    portId = alarmData.almDetail & 0xFF;

    // translate switch port to me id
    if (GOS_OK != pptp_eth_uni_switch_port_to_me_id(portId, pInstanceID))
        return GOS_ERR_PARAM;

    slotId = *pInstanceID >> 8;

    if (GOS_OK != mib_alarm_table_get(tableIndex, *pInstanceID, &alarmTable))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Get alarm table fail: %s, 0x%x",
            MIB_GetTableName(tableIndex), *pInstanceID);

        return GOS_FAIL;
    }

    // update alarm status if it has being changed
    mib_alarm_table_update(&alarmTable, &alarmData, pIsUpdated);

    if (*pIsUpdated)
    {
        if (GOS_OK != mib_alarm_table_set(tableIndex, *pInstanceID, &alarmTable))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set alarm table fail: %s, 0x%x",
                MIB_GetTableName(tableIndex), *pInstanceID);

            return GOS_FAIL;
        }

        // find the entry data by using the instanceID
        pEntry = mib_FindEntryByInstanceID(tableIndex, *pInstanceID);
        if (!pEntry)
            return GOS_FAIL;
        pMibPptpEthUni = pEntry->pData;
        if (!pMibPptpEthUni)
            return GOS_FAIL;

        // correct the circuit pack instanceID to the slot it belongs to
        cpInstanceID = TXC_CARDHLD_SLOT_TYPE_INTEGRATED | slotId;

        // check if notifications are suppressed by parent's admin state
        omci_is_notify_suppressed_by_circuitpack(cpInstanceID, &isSuppressed);

        if (isSuppressed)
            *pIsUpdated = FALSE;
        else
        {
            // check if notifications are suppressed by self's admin state
            if (OMCI_ME_ATTR_ADMIN_STATE_LOCK == pMibPptpEthUni->AdminState)
                *pIsUpdated = FALSE;

            // check if notifications are suppressed by ARC
            if (OMCI_ME_ATTR_ARC_ENABLED == pMibPptpEthUni->ARC)
                *pIsUpdated = FALSE;
        }

        if (OMCI_ALM_NBR_ETHERNET_LAN_LOS == alarmData.almNumber)
        {
            MIB_TABLE_ETHUNI_T  mibPptpEthUni;

            mibPptpEthUni.EntityID = pMibPptpEthUni->EntityID;

            // read out the entry data
            if (GOS_OK == MIB_Get(tableIndex, &mibPptpEthUni, sizeof(MIB_TABLE_ETHUNI_T)))
            {
                // update operational state
                mibPptpEthUni.OpState = (OMCI_ALM_STS_DECLARE == alarmData.almStatus)
                        ? OMCI_ME_ATTR_OP_STATE_DISABLED : OMCI_ME_ATTR_OP_STATE_ENABLED;

                // update configuration ind
                mibPptpEthUni.DuplexInd = PPTP_ETH_UNI_CFG_IND_UNKNOWN;

                // update speed/duplex if link is up
                if (OMCI_ME_ATTR_OP_STATE_ENABLED == mibPptpEthUni.OpState)
                {
                    // reset portId to alarm detail
                    portId = alarmData.almDetail & 0xFF;

                    pptp_eth_uni_speed_duplex_updater(portId, &mibPptpEthUni);
                }

                // invoke AVC callback and update attribute value
                MIB_Set(tableIndex, &mibPptpEthUni, sizeof(MIB_TABLE_ETHUNI_T));
            }
        }
    }

    return GOS_OK;
}

static GOS_ERROR_CODE pptp_eth_uni_avc_cb(MIB_TABLE_INDEX   tableIndex,
                                            void            *pOldRow,
                                            void            *pNewRow,
                                            MIB_ATTRS_SET   *pAttrsSet,
                                            MIB_OPERA_TYPE  operationType)
{
    MIB_ATTR_INDEX      attrIndex;
    UINT32              attrSize, i;
    MIB_ATTRS_SET       avcAttrSet;
    BOOL                isSuppressed;
    omci_me_instance_t  instanceID;

    if (MIB_SET != operationType && MIB_ADD != operationType)
        return GOS_OK;

    if (!pNewRow || !pAttrsSet)
        return GOS_ERR_PARAM;

    // read out the instanceID
    MIB_GetAttrFromBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &instanceID, pNewRow, sizeof(omci_me_instance_t));

    // correct the instanceID to the slot it belongs to
    instanceID = TXC_CARDHLD_SLOT_TYPE_INTEGRATED | (instanceID >> 8);

    // check if notifications are suppressed
    omci_is_notify_suppressed_by_circuitpack(instanceID, &isSuppressed);
    if (!isSuppressed)
    {
        if (MIB_SET == operationType && pOldRow)
        {
            UINT8 adminState;

            MIB_GetAttrFromBuf(tableIndex, MIB_TABLE_ETHUNI_ADMINSTATE_INDEX, &adminState, pOldRow, sizeof(UINT8));

            if (OMCI_ME_ATTR_ADMIN_STATE_LOCK == adminState)
                isSuppressed = TRUE;
        }
    }

    if (isSuppressed)
        MIB_ClearAttrSet(&avcAttrSet);
    else
    {
        avcAttrSet = *pAttrsSet;

        for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0;
                i < MIB_GetTableAttrNum(tableIndex); i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
        {
            if (!MIB_IsInAttrSet(pAttrsSet, attrIndex))
                continue;

            attrSize = MIB_GetAttrSize(tableIndex, attrIndex);

            // sense type AVC has special handling according to G.988 9.5.1
            if (MIB_TABLE_ETHUNI_SENSEDTYPE_INDEX == attrIndex)
            {
                // only report sense type AVC on MIB add
                if (MIB_ADD == operationType)
                {
                    UINT8 newValue, oldValue;

                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &oldValue, pNewRow, attrSize);
                    newValue = oldValue;

                    if (OMCI_PLUGIN_UNIT_TYPE_10_100BASE_T == oldValue)
                        newValue = PPTP_ETH_UNI_SENSE_TYPE_AVC_100BASET;
                    else if (OMCI_PLUGIN_UNIT_TYPE_10_100_1000BASE_T == oldValue)
                        newValue = PPTP_ETH_UNI_SENSE_TYPE_AVC_GIGABIT_ETHERNET;

                    if (newValue != oldValue)
                    {
                        MIB_SetAttrToBuf(tableIndex, attrIndex, &newValue, pNewRow, attrSize);
                        MIB_SetAttrSet(&avcAttrSet, attrIndex);
                    }
                }
                else
                    MIB_UnSetAttrSet(&avcAttrSet, attrIndex);
            }
        }
    }

    if (avcAttrSet != *pAttrsSet)
        *pAttrsSet = avcAttrSet;

    return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibEthUniTableInfo.Name = "EthUni";
    gMibEthUniTableInfo.ShortName = "PPTP";
    gMibEthUniTableInfo.Desc = "PPTP Ethernet UNI";
    gMibEthUniTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_PPTP_ETHERNET_UNI);
    gMibEthUniTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibEthUniTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibEthUniTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibEthUniTableInfo.pAttributes = &(gMibEthUniAttrInfo[0]);


    gMibEthUniTableInfo.attrNum = MIB_TABLE_ETHUNI_ATTR_NUM;
    gMibEthUniTableInfo.entrySize = sizeof(MIB_TABLE_ETHUNI_T);
    gMibEthUniTableInfo.pDefaultRow = &gMibEthUniDefRow;


    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_EXPECTEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ExpectedType";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_SENSEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "SensedType";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_AUTODECTECTCFG_INDEX - MIB_TABLE_FIRST_INDEX].Name = "AutoDectectCfg";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ETHLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EthLoopbackCfg";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "AdminState";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "OpState";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_DUPLEXIND_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DuplexInd";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_MAXFRAMESIZE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "MaxFrameSize";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_DTEORDCEIND_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DTEorDCEInd";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PAUSETIME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PauseTime";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_BRIDGEDORIPIND_INDEX - MIB_TABLE_FIRST_INDEX].Name = "BridgedorIPInd";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ARC_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ARC";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ARCInterval";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PPPOEFILTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PppoeFilter";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_POWERCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PowerControl";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PHYSICAL_PORT_ID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PhysicalPortId";

    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_EXPECTEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Expected type";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_SENSEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Sensed type";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_AUTODECTECTCFG_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Auto detection configuration";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ETHLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Ethernet loopback configuration";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Administrative state";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Operational state";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_DUPLEXIND_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Duplex Indication";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_MAXFRAMESIZE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Max Frame Size";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_DTEORDCEIND_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "DTE or DCE Indication";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PAUSETIME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Pause Time";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_BRIDGEDORIPIND_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Bridged or IP router";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ARC_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Alarm Report Control";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "ARC Interval";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PPPOEFILTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "PPPoE Filter";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_POWERCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Power Control";
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PHYSICAL_PORT_ID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Physical port ID";

    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_EXPECTEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_SENSEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_AUTODECTECTCFG_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ETHLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_DUPLEXIND_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_MAXFRAMESIZE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_DTEORDCEIND_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PAUSETIME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_BRIDGEDORIPIND_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ARC_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PPPOEFILTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_POWERCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PHYSICAL_PORT_ID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;

    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_EXPECTEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_SENSEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_AUTODECTECTCFG_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ETHLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_DUPLEXIND_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_MAXFRAMESIZE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_DTEORDCEIND_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PAUSETIME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_BRIDGEDORIPIND_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ARC_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PPPOEFILTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_POWERCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PHYSICAL_PORT_ID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;

    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_EXPECTEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_SENSEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_AUTODECTECTCFG_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ETHLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_DUPLEXIND_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_MAXFRAMESIZE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_DTEORDCEIND_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PAUSETIME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_BRIDGEDORIPIND_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ARC_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PPPOEFILTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_POWERCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PHYSICAL_PORT_ID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_EXPECTEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_SENSEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_AUTODECTECTCFG_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ETHLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_DUPLEXIND_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_MAXFRAMESIZE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_DTEORDCEIND_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PAUSETIME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_BRIDGEDORIPIND_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ARC_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PPPOEFILTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_POWERCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PHYSICAL_PORT_ID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_EXPECTEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_SENSEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_AUTODECTECTCFG_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ETHLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_DUPLEXIND_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_MAXFRAMESIZE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_DTEORDCEIND_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PAUSETIME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_BRIDGEDORIPIND_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ARC_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PPPOEFILTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_POWERCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PHYSICAL_PORT_ID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_EXPECTEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_SENSEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_AUTODECTECTCFG_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ETHLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_DUPLEXIND_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_MAXFRAMESIZE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_DTEORDCEIND_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PAUSETIME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_BRIDGEDORIPIND_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ARC_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PPPOEFILTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_POWERCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PHYSICAL_PORT_ID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_EXPECTEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_SENSEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_AUTODECTECTCFG_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ETHLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_DUPLEXIND_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_MAXFRAMESIZE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_DTEORDCEIND_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PAUSETIME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_BRIDGEDORIPIND_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ARC_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PPPOEFILTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_POWERCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PHYSICAL_PORT_ID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_EXPECTEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_SENSEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_AUTODECTECTCFG_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ETHLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_DUPLEXIND_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_MAXFRAMESIZE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_DTEORDCEIND_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PAUSETIME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_BRIDGEDORIPIND_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ARC_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PPPOEFILTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_POWERCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibEthUniAttrInfo[MIB_TABLE_ETHUNI_PHYSICAL_PORT_ID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_PRIVATE;


    memset(&(gMibEthUniDefRow.EntityID), 0x00, sizeof(gMibEthUniDefRow.EntityID));
    memset(&(gMibEthUniDefRow.ExpectedType), 0x00, sizeof(gMibEthUniDefRow.ExpectedType));
    memset(&(gMibEthUniDefRow.SensedType), 0x00, sizeof(gMibEthUniDefRow.SensedType));
    gMibEthUniDefRow.AutoDectectCfg = PPTP_ETH_UNI_AUTO_DETECT_CFG_AUTO_AUTO;
    memset(&(gMibEthUniDefRow.EthLoopbackCfg), 0x00, sizeof(gMibEthUniDefRow.EthLoopbackCfg));
    gMibEthUniDefRow.AdminState = OMCI_ME_ATTR_ADMIN_STATE_UNLOCK;
    gMibEthUniDefRow.OpState = OMCI_ME_ATTR_OP_STATE_DISABLED;
    memset(&(gMibEthUniDefRow.DuplexInd), 0x00, sizeof(gMibEthUniDefRow.DuplexInd));
    gMibEthUniDefRow.MaxFrameSize = 1518;
    memset(&(gMibEthUniDefRow.DTEorDCEInd), 0x00, sizeof(gMibEthUniDefRow.DTEorDCEInd));
    memset(&(gMibEthUniDefRow.PauseTime), 0xFF, sizeof(gMibEthUniDefRow.PauseTime));
    memset(&(gMibEthUniDefRow.BridgedorIPInd), 0x00, sizeof(gMibEthUniDefRow.BridgedorIPInd));
    gMibEthUniDefRow.BridgedorIPInd = PPTP_ETH_UNI_BRIIPIND_CIRCUITPACK;
    memset(&(gMibEthUniDefRow.ARC), 0x00, sizeof(gMibEthUniDefRow.ARC));
    gMibEthUniDefRow.ARCInterval = 0;
    memset(&(gMibEthUniDefRow.PppoeFilter), 0x00, sizeof(gMibEthUniDefRow.PppoeFilter));
    memset(&(gMibEthUniDefRow.PowerControl), 0x00, sizeof(gMibEthUniDefRow.PowerControl));
    gMibEthUniDefRow.BridgedorIPInd = PPTP_ETH_UNI_BRIIPIND_CIRCUITPACK;

    memset(&gMibEthUniOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibEthUniOper.meOperDrvCfg = EthUniDrvCfg;
    gMibEthUniOper.meOperConnCheck = EthUniConnCheck;
    gMibEthUniOper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibEthUniOper.meOperConnCfg = NULL;
    gMibEthUniOper.meOperAvlTreeAdd = EthUniAvlTreeAdd;
    gMibEthUniOper.meOperAvlTreeDel = EthUniAvlTreeDel;
    gMibEthUniOper.meOperAlarmHandler = pptp_eth_uni_alarm_handler;

    MIB_TABLE_ETHUNI_INDEX = tableId;
    MIB_InfoRegister(tableId,&gMibEthUniTableInfo,&gMibEthUniOper);
    MIB_RegisterCallback(tableId, NULL, pptp_eth_uni_avc_cb);

    return GOS_OK;
}
