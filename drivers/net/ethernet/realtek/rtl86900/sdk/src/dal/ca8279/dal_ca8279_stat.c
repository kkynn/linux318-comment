/*
 * Copyright (C) 2013 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * $Revision: 43213 $
 * $Date: 2013-09-27 15:59:36 +0800 (Fri, 27 Sep 2013) $
 *
 * Purpose : Definition of Statistic API
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) Statistic Counter Reset
 *           (2) Statistic Counter Get
 *
 */

#include <common/rt_type.h>
#include <rtk/port.h>
#include <dal/ca8279/dal_ca8279.h>
#include <dal/ca8279/dal_ca8279_stat.h>
#include <osal/time.h>

#include <cortina-api/include/port.h>
#include <cortina-api/include/eth_port.h>
#include <aal/include/aal_ni_l2.h>

/*
 * Include Files
 */

/*
 * Symbol Definition
 */


/*
 * Data Declaration
 */
static uint32    stat_init = INIT_NOT_COMPLETED;

static rtk_stat_port_cntr_t xgePortCntrs[2];

/*
 * Function Declaration
 */

/* Module Name : STAT */

/* Function Name:
 *      dal_ca8279_stat_init
 * Description:
 *      Initialize stat module of the specified device.
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_STAT_GLOBAL_CNTR_FAIL - Could not retrieve/reset Global Counter
 *      RT_ERR_STAT_PORT_CNTR_FAIL   - Could not retrieve/reset Port Counter
 * Note:
 *      Must initialize stat module before calling any stat APIs.
 */
int32
dal_ca8279_stat_init(void)
{
    int32 ret;
    rtk_port_t port;

    stat_init = INIT_COMPLETED;

    HAL_SCAN_ALL_PORT(port)
    {
        if((ret = dal_ca8279_stat_port_reset(port)) != RT_ERR_OK)
        {
            RT_ERR(ret, (MOD_STAT | MOD_DAL), "");
        }
    }

    return RT_ERR_OK;
}

/* Function Name:
 *      dal_ca8279_stat_port_reset
 * Description:
 *      Reset the specified port counters in the specified device.
 * Input:
 *      port - port id
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID             - invalid port id
 *      RT_ERR_STAT_PORT_CNTR_FAIL - Could not retrieve/reset Port Counter
 * Note:
 *      None
 */
int32
dal_ca8279_stat_port_reset(rtk_port_t port)
{
    ca_status_t ret=CA_E_OK;
    ca_port_id_t port_id;
    ca_boolean_t read_keep;
    ca_eth_port_stat_t data;

    /* check Init status */
    RT_INIT_CHK(stat_init);

    port_id = RTK2CA_PORT_ID(port);

    /* parameter check */
    RT_PARAM_CHK((port_id == CAERR_PORT_ID), RT_ERR_PORT_ID);
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    
    if(port == AAL_NI_PORT_XGE1 || port == AAL_NI_PORT_XGE2) //poort 6 and port 7
    {
        aal_ni_mib_stats_t  stats ; 

        memset(&stats,0x00,sizeof(stats));

        if((ret = aal_ni_xge_mac_mib_get(0,port-AAL_NI_PORT_XGE1,&stats)) != CA_E_OK)
        {
            RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
            return RT_ERR_FAILED;
        }

        memset(&xgePortCntrs[port-AAL_NI_PORT_XGE1],0,sizeof(rtk_stat_port_cntr_t));
    }
    else
    {
        read_keep = 0;

        memset(&data, 0, sizeof(ca_eth_port_stat_t));

        if((ret = ca_eth_port_stat_get(0,port_id,read_keep,&data)) != CA_E_OK)
        {
            RT_ERR(ret, (MOD_STAT | MOD_DAL), "");
            return RT_ERR_FAILED;
        }
    }

    return RT_ERR_OK;
}

/* Function Name:
 *      dal_ca8279_stat_port_get
 * Description:
 *      Get one specified port counter.
 * Input:
 *      port     - port id
 *      cntrIdx - specified port counter index
 * Output:
 *      pCntr    - pointer buffer of counter value
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID             - invalid port id
 *      RT_ERR_NULL_POINTER        - input parameter may be null pointer
 *      RT_ERR_STAT_PORT_CNTR_FAIL - Could not retrieve/reset Port Counter
 * Note:
 *      None
 */
int32
dal_ca8279_stat_port_get(rtk_port_t port, rtk_stat_port_type_t cntrIdx, uint64 *pCntr)
{
    int32 ret;
    rtk_stat_port_cntr_t portCntrs;

    RT_DBG(LOG_DEBUG, (MOD_DAL|MOD_STAT),"%s",__FUNCTION__);

    /* check Init status */
    RT_INIT_CHK(stat_init);

    /* parameter check */
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_INPUT);
    RT_PARAM_CHK((MIB_PORT_CNTR_END <= cntrIdx), RT_ERR_INPUT);
    RT_PARAM_CHK((NULL == pCntr), RT_ERR_NULL_POINTER);

    if ((ret = dal_ca8279_stat_port_getAll(port,&portCntrs)) != RT_ERR_OK)
    {
        RT_ERR(ret, (MOD_DAL|MOD_STAT), "");
        return ret;
    }

    *pCntr = 0;

    switch(cntrIdx)
    {
        case DOT1D_TP_PORT_IN_DISCARDS_INDEX:
            *pCntr = portCntrs.ifInDiscards;
            break;
        case IF_IN_OCTETS_INDEX:
            *pCntr = portCntrs.ifInOctets;
            break;
        case IF_IN_UCAST_PKTS_INDEX:
            *pCntr = portCntrs.ifInUcastPkts;
            break;
        case IF_IN_MULTICAST_PKTS_INDEX:
            *pCntr = portCntrs.ifInMulticastPkts;
            break;
        case IF_IN_BROADCAST_PKTS_INDEX:
            *pCntr = portCntrs.ifInBroadcastPkts;
            break;
        case IF_OUT_OCTETS_INDEX:
            *pCntr = portCntrs.ifOutOctets;
            break;
        case IF_OUT_UCAST_PKTS_CNT_INDEX:
            *pCntr = portCntrs.ifOutUcastPkts;
            break;
        case IF_OUT_MULTICAST_PKTS_CNT_INDEX:
            *pCntr = portCntrs.ifOutMulticastPkts;
            break;
        case IF_OUT_BROADCAST_PKTS_CNT_INDEX:
            *pCntr = portCntrs.ifOutBrocastPkts;
            break;
        case IF_OUT_DISCARDS_INDEX:
            *pCntr = portCntrs.ifOutDiscards;
            break;
        case DOT3_STATS_SINGLE_COLLISION_FRAMES_INDEX:
            *pCntr = portCntrs.dot3StatsSingleCollisionFrames;
            break;
        case DOT3_STATS_MULTIPLE_COLLISION_FRAMES_INDEX:
            *pCntr = portCntrs.dot3StatsMultipleCollisionFrames;
            break;
        case DOT3_STATS_DEFERRED_TRANSMISSIONS_INDEX:
            *pCntr = portCntrs.dot3StatsDeferredTransmissions;
            break;
        case DOT3_STATS_LATE_COLLISIONS_INDEX:
            *pCntr = portCntrs.dot3StatsLateCollisions;
            break;
        case DOT3_STATS_EXCESSIVE_COLLISIONS_INDEX:
            *pCntr = portCntrs.dot3StatsExcessiveCollisions;
            break;
        case DOT3_STATS_SYMBOL_ERRORS_INDEX:
            *pCntr = portCntrs.dot3StatsSymbolErrors;
            break;
        case DOT3_CONTROL_IN_UNKNOWN_OPCODES_INDEX:
            *pCntr = portCntrs.dot3ControlInUnknownOpcodes;
            break;
        case DOT3_IN_PAUSE_FRAMES_INDEX:
            *pCntr = portCntrs.dot3InPauseFrames;
            break;
        case DOT3_OUT_PAUSE_FRAMES_INDEX:
            *pCntr = portCntrs.dot3OutPauseFrames;
            break;
        case ETHER_STATS_DROP_EVENTS_INDEX:
            *pCntr = portCntrs.etherStatsDropEvents;
            break;
        case ETHER_STATS_TX_BROADCAST_PKTS_INDEX:
            *pCntr = portCntrs.etherStatsTxBcastPkts;
            break;
        case ETHER_STATS_TX_MULTICAST_PKTS_INDEX:
            *pCntr = portCntrs.etherStatsTxMcastPkts;
            break;
        case ETHER_STATS_CRC_ALIGN_ERRORS_INDEX:
            *pCntr = portCntrs.etherStatsCRCAlignErrors;
            break;
        case ETHER_STATS_TX_UNDER_SIZE_PKTS_INDEX:
            *pCntr = portCntrs.etherStatsTxUndersizePkts;
            break;
        case ETHER_STATS_RX_UNDER_SIZE_PKTS_INDEX:
            *pCntr = portCntrs.etherStatsRxUndersizePkts;
            break;
        case ETHER_STATS_TX_OVERSIZE_PKTS_INDEX:
            *pCntr = portCntrs.etherStatsTxOversizePkts;
            break;
        case ETHER_STATS_RX_OVERSIZE_PKTS_INDEX:
            *pCntr = portCntrs.etherStatsRxOversizePkts;
            break;
        case ETHER_STATS_FRAGMENTS_INDEX:
            *pCntr = portCntrs.etherStatsFragments;
            break;
        case ETHER_STATS_JABBERS_INDEX:
            *pCntr = portCntrs.etherStatsJabbers;
            break;
        case ETHER_STATS_COLLISIONS_INDEX:
            *pCntr = portCntrs.etherStatsCollisions;
            break;
        case ETHER_STATS_TX_PKTS_64OCTETS_INDEX:
            *pCntr = portCntrs.etherStatsTxPkts64Octets;
            break;
        case ETHER_STATS_RX_PKTS_64OCTETS_INDEX:
            *pCntr = portCntrs.etherStatsRxPkts64Octets;
            break;
        case ETHER_STATS_TX_PKTS_65TO127OCTETS_INDEX:
            *pCntr = portCntrs.etherStatsTxPkts65to127Octets;
            break;
        case ETHER_STATS_RX_PKTS_65TO127OCTETS_INDEX:
            *pCntr = portCntrs.etherStatsRxPkts65to127Octets;
            break;
        case ETHER_STATS_TX_PKTS_128TO255OCTETS_INDEX:
            *pCntr = portCntrs.etherStatsTxPkts128to255Octets;
            break;
        case ETHER_STATS_RX_PKTS_128TO255OCTETS_INDEX:
            *pCntr = portCntrs.etherStatsRxPkts128to255Octets;
            break;
        case ETHER_STATS_TX_PKTS_256TO511OCTETS_INDEX:
            *pCntr = portCntrs.etherStatsTxPkts256to511Octets;
            break;
        case ETHER_STATS_RX_PKTS_256TO511OCTETS_INDEX:
            *pCntr = portCntrs.etherStatsRxPkts256to511Octets;
            break;
        case ETHER_STATS_TX_PKTS_512TO1023OCTETS_INDEX:
            *pCntr = portCntrs.etherStatsTxPkts512to1023Octets;
            break;
        case ETHER_STATS_RX_PKTS_512TO1023OCTETS_INDEX:
            *pCntr = portCntrs.etherStatsRxPkts512to1023Octets;
            break;
        case ETHER_STATS_TX_PKTS_1024TO1518OCTETS_INDEX:
            *pCntr = portCntrs.etherStatsTxPkts1024to1518Octets;
            break;
        case ETHER_STATS_RX_PKTS_1024TO1518OCTETS_INDEX:
            *pCntr = portCntrs.etherStatsRxPkts1024to1518Octets;
            break;
        case ETHER_STATS_TX_PKTS_1519TOMAXOCTETS_INDEX:
            *pCntr = portCntrs.etherStatsTxPkts1519toMaxOctets;
            break;
        case ETHER_STATS_RX_PKTS_1519TOMAXOCTETS_INDEX:
            *pCntr = portCntrs.etherStatsRxPkts1519toMaxOctets;
            break;
        case IN_OAM_PDU_PKTS_INDEX:
            *pCntr = portCntrs.inOampduPkts;
            break;
        case OUT_OAM_PDU_PKTS_INDEX:
            *pCntr = portCntrs.outOampduPkts;
            break;
        default:
            return RT_ERR_INPUT;;
    }
    return RT_ERR_OK;
}

/* Function Name:
 *      dal_ca8279_stat_port_getAll
 * Description:
 *      Get all counters of one specified port in the specified device.
 * Input:
 *      port        - port id
 * Output:
 *      pPortCntrs - pointer buffer of counter value
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_FAILED
 *      RT_ERR_PORT_ID             - invalid port id
 *      RT_ERR_NULL_POINTER        - input parameter may be null pointer
 *      RT_ERR_STAT_PORT_CNTR_FAIL - Could not retrieve/reset Port Counter
 * Note:
 *      None
 */
int32
dal_ca8279_stat_port_getAll(rtk_port_t port, rtk_stat_port_cntr_t *pPortCntrs)
{
    ca_status_t ret=CA_E_OK;
    ca_port_id_t port_id;
    ca_boolean_t read_keep;

    /* check Init status */
    RT_INIT_CHK(stat_init);

    port_id = RTK2CA_PORT_ID(port);

    /* parameter check */
    RT_PARAM_CHK((port_id == CAERR_PORT_ID), RT_ERR_PORT_ID);
    RT_PARAM_CHK(!HAL_IS_PORT_EXIST(port), RT_ERR_PORT_ID);
    RT_PARAM_CHK((NULL == pPortCntrs), RT_ERR_NULL_POINTER);

    read_keep = 1;

    memset(pPortCntrs, 0, sizeof(rtk_stat_port_cntr_t));

    if(port == AAL_NI_PORT_XGE1 || port == AAL_NI_PORT_XGE2) //poort 6 and port 7
    {
        aal_ni_mib_stats_t  stats ; 

        memset(&stats,0x00,sizeof(stats));

        if((ret = aal_ni_xge_mac_mib_get(0,port-AAL_NI_PORT_XGE1,&stats)) != CA_E_OK)
        {
            RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
            return RT_ERR_FAILED;
        }
        
        xgePortCntrs[port-AAL_NI_PORT_XGE1].ifInOctets                           += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].ifInUcastPkts                        += stats.rx_uc_pkt_cnt;        /* received unicast frames                                                   */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].ifInMulticastPkts                    += stats.rx_mc_frame_cnt;        /* received multicast frames                                                 */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].ifInBroadcastPkts                    += stats.rx_bc_frame_cnt;        /* received broadcast frames                                                 */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].ifInDiscards                         += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].ifOutOctets                          += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].ifOutDiscards                        += 0; 
        xgePortCntrs[port-AAL_NI_PORT_XGE1].ifOutUcastPkts                       += stats.tx_uc_pkt_cnt;        /* transmitted unicast frames                                                */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].ifOutMulticastPkts                   += stats.tx_mc_frame_cnt;        /* transmitted multicast frames                                              */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].ifOutBrocastPkts                     += stats.tx_bc_frame_cnt;        /* transmitted broadcast frames                                              */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].dot1dBasePortDelayExceededDiscards   += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].dot1dTpPortInDiscards                += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].dot1dTpHcPortInDiscards              += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].dot3InPauseFrames                    += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].dot3OutPauseFrames                   += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].dot3OutPauseOnFrames                 += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].dot3StatsAligmentErrors              += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].dot3StatsFCSErrors                   += stats.rx_crc_error_frame_cnt;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].dot3StatsSingleCollisionFrames       += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].dot3StatsMultipleCollisionFrames     += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].dot3StatsDeferredTransmissions       += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].dot3StatsLateCollisions              += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].dot3StatsExcessiveCollisions         += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].dot3StatsFrameTooLongs               += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].dot3StatsSymbolErrors                += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].dot3ControlInUnknownOpcodes          += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsDropEvents                 += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsOctets                     += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsBcastPkts                  += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsMcastPkts                  += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsUndersizePkts              += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsOversizePkts               += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsFragments                  += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsJabbers                    += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsCollisions                 += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsCRCAlignErrors             += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsPkts64Octets               += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsPkts65to127Octets          += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsPkts128to255Octets         += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsPkts256to511Octets         += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsPkts512to1023Octets        += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsPkts1024to1518Octets       += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsTxOctets                   += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsTxUndersizePkts            += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsTxOversizePkts             += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsTxPkts64Octets             += stats.tx_frame_64_cnt;        /* transmitted frames with 64 bytes                                          */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsTxPkts65to127Octets        += stats.tx_frame_65to127_cnt;    /* transmitted frames with 65 ~ 127 bytes                                    */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsTxPkts128to255Octets       += stats.tx_frame_128to255_cnt;   /* transmitted frames with 128 ~ 255 bytes                                   */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsTxPkts256to511Octets       += stats.tx_frame_256to511_cnt;   /* transmitted frames with 256 ~ 511 bytes                                   */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsTxPkts512to1023Octets      += stats.tx_frame_512to1023_cnt;  /* transmitted frames with 512 ~ 1023 bytes                                  */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsTxPkts1024to1518Octets     += stats.tx_frame_1024to1518_cnt; /* transmitted frames with 1024 ~ 1518 bytes                                 */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsTxPkts1519toMaxOctets      += stats.tx_frame_1519to2100_cnt + stats.tx_frame_2101to9200_cnt + stats.tx_frame_9201tomax_cnt;  /* transmitted frames of which length is equal to or greater than 1519 bytes */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsTxBcastPkts                += stats.tx_bc_frame_cnt;        /* transmitted broadcast frames                                              */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsTxMcastPkts                += stats.tx_mc_frame_cnt;        /* transmitted multicast frames                                              */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsTxFragments                += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsTxJabbers                  += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsTxCRCAlignErrors           += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsRxUndersizePkts            += stats.rx_undersize_frame_cnt; /* received frames undersize frame without FCS err               */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsRxUndersizeDropPkts        += stats.rx_runt_frame_cnt - stats.rx_undersize_frame_cnt;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsRxOversizePkts             += 0;
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsRxPkts64Octets             += stats.rx_frame_64_cnt;        /* received frames with 64 bytes                                             */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsRxPkts65to127Octets        += stats.rx_frame_65to127_cnt;    /* received frames with 65 ~ 127 bytes                                       */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsRxPkts128to255Octets       += stats.rx_frame_128to255_cnt;   /* received frames with 128 ~ 255 bytes                                      */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsRxPkts256to511Octets       += stats.rx_frame_256to511_cnt;   /* received frames with 256 ~ 511 bytes                                      */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsRxPkts512to1023Octets      += stats.rx_frame_512to1023_cnt;  /* received frames with 512 ~ 1023 bytes                                     */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsRxPkts1024to1518Octets     += stats.rx_frame_1024to1518_cnt; /* received frames with 1024 ~ 1518 bytes                                    */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].etherStatsRxPkts1519toMaxOctets      += stats.rx_frame_1519to2100_cnt + stats.rx_frame_2101to9200_cnt + stats.rx_frame_9201tomax_cnt;  /* received frames of which length is equal to or greater than 1519 bytes    */
        xgePortCntrs[port-AAL_NI_PORT_XGE1].inOampduPkts                         += 0; //TBD waiting for Saturn Chip
        xgePortCntrs[port-AAL_NI_PORT_XGE1].outOampduPkts                        += 0; //TBD waiting for Saturn Chip

        memcpy(pPortCntrs,&xgePortCntrs[port-AAL_NI_PORT_XGE1],sizeof(rtk_stat_port_cntr_t));
    }
    else
    {
        ca_eth_port_stat_t data;

        memset(&data, 0, sizeof(ca_eth_port_stat_t));

        if((ret = ca_eth_port_stat_get(0,port_id,read_keep,&data)) != CA_E_OK)
        {
            RT_ERR(ret, (MOD_L2 | MOD_DAL), "");
            return RT_ERR_FAILED;
        }

        pPortCntrs->ifInOctets                           = 0;
        pPortCntrs->ifInUcastPkts                        = data.rx_uc_frames;        /* received unicast frames                                                   */
        pPortCntrs->ifInMulticastPkts                    = data.rx_mc_frames;        /* received multicast frames                                                 */
        pPortCntrs->ifInBroadcastPkts                    = data.rx_bc_frames;        /* received broadcast frames                                                 */
        pPortCntrs->ifInDiscards                         = 0;
        pPortCntrs->ifOutOctets                          = 0;
        pPortCntrs->ifOutDiscards                        = 0; 
        pPortCntrs->ifOutUcastPkts                       = data.tx_uc_frames;        /* transmitted unicast frames                                                */
        pPortCntrs->ifOutMulticastPkts                   = data.tx_mc_frames;        /* transmitted multicast frames                                              */
        pPortCntrs->ifOutBrocastPkts                     = data.tx_bc_frames;        /* transmitted broadcast frames                                              */
        pPortCntrs->dot1dBasePortDelayExceededDiscards   = 0;
        pPortCntrs->dot1dTpPortInDiscards                = 0;
        pPortCntrs->dot1dTpHcPortInDiscards              = 0;
        pPortCntrs->dot3InPauseFrames                    = 0;
        pPortCntrs->dot3OutPauseFrames                   = 0;
        pPortCntrs->dot3OutPauseOnFrames                 = 0;
        pPortCntrs->dot3StatsAligmentErrors              = data.phy_stats.alignmentErrors;
        pPortCntrs->dot3StatsFCSErrors                   = data.rx_fcs_error_frames;
        pPortCntrs->dot3StatsSingleCollisionFrames       = data.phy_stats.single_collision_frames;
        pPortCntrs->dot3StatsMultipleCollisionFrames     = data.phy_stats.multiple_collision_frames;
        pPortCntrs->dot3StatsDeferredTransmissions       = data.phy_stats.frames_with_deferredXmissions;
        pPortCntrs->dot3StatsLateCollisions              = data.phy_stats.late_collisions;
        pPortCntrs->dot3StatsExcessiveCollisions         = 0;
        pPortCntrs->dot3StatsFrameTooLongs               = 0;
        pPortCntrs->dot3StatsSymbolErrors                = data.phy_stats.symbol_error_during_carrier;
        pPortCntrs->dot3ControlInUnknownOpcodes          = data.phy_stats.unsupported_opcodes_received;
        pPortCntrs->etherStatsDropEvents                 = 0;
        pPortCntrs->etherStatsOctets                     = 0;
        pPortCntrs->etherStatsBcastPkts                  = 0;
        pPortCntrs->etherStatsMcastPkts                  = 0;
        pPortCntrs->etherStatsUndersizePkts              = 0;
        pPortCntrs->etherStatsOversizePkts               = 0;
        pPortCntrs->etherStatsFragments                  = 0;
        pPortCntrs->etherStatsJabbers                    = 0;
        pPortCntrs->etherStatsCollisions                 = 0;
        pPortCntrs->etherStatsCRCAlignErrors             = 0;
        pPortCntrs->etherStatsPkts64Octets               = 0;
        pPortCntrs->etherStatsPkts65to127Octets          = 0;
        pPortCntrs->etherStatsPkts128to255Octets         = 0;
        pPortCntrs->etherStatsPkts256to511Octets         = 0;
        pPortCntrs->etherStatsPkts512to1023Octets        = 0;
        pPortCntrs->etherStatsPkts1024to1518Octets       = 0;
        pPortCntrs->etherStatsTxOctets                   = 0;
        pPortCntrs->etherStatsTxUndersizePkts            = 0;
        pPortCntrs->etherStatsTxOversizePkts             = 0;
        pPortCntrs->etherStatsTxPkts64Octets             = data.tx_64_frames;        /* transmitted frames with 64 bytes                                          */
        pPortCntrs->etherStatsTxPkts65to127Octets        = data.tx_65_127_frames;    /* transmitted frames with 65 ~ 127 bytes                                    */
        pPortCntrs->etherStatsTxPkts128to255Octets       = data.tx_128_255_frames;   /* transmitted frames with 128 ~ 255 bytes                                   */
        pPortCntrs->etherStatsTxPkts256to511Octets       = data.tx_256_511_frames;   /* transmitted frames with 256 ~ 511 bytes                                   */
        pPortCntrs->etherStatsTxPkts512to1023Octets      = data.tx_512_1023_frames;  /* transmitted frames with 512 ~ 1023 bytes                                  */
        pPortCntrs->etherStatsTxPkts1024to1518Octets     = data.tx_1024_1518_frames; /* transmitted frames with 1024 ~ 1518 bytes                                 */
        pPortCntrs->etherStatsTxPkts1519toMaxOctets      = data.tx_1519_max_frames;  /* transmitted frames of which length is equal to or greater than 1519 bytes */
        pPortCntrs->etherStatsTxBcastPkts                = data.tx_bc_frames;        /* transmitted broadcast frames                                              */
        pPortCntrs->etherStatsTxMcastPkts                = data.tx_mc_frames;        /* transmitted multicast frames                                              */
        pPortCntrs->etherStatsTxFragments                = 0;
        pPortCntrs->etherStatsTxJabbers                  = 0;
        pPortCntrs->etherStatsTxCRCAlignErrors           = 0;
        pPortCntrs->etherStatsRxUndersizePkts            = data.rx_undersize_frames; /* received frames undersize frame without FCS err               */
        pPortCntrs->etherStatsRxUndersizeDropPkts        = data.rx_runt_frames - data.rx_undersize_frames;
        pPortCntrs->etherStatsRxOversizePkts             = 0;
        pPortCntrs->etherStatsRxPkts64Octets             = data.rx_64_frames;        /* received frames with 64 bytes                                             */
        pPortCntrs->etherStatsRxPkts65to127Octets        = data.rx_65_127_frames;    /* received frames with 65 ~ 127 bytes                                       */
        pPortCntrs->etherStatsRxPkts128to255Octets       = data.rx_128_255_frames;   /* received frames with 128 ~ 255 bytes                                      */
        pPortCntrs->etherStatsRxPkts256to511Octets       = data.rx_256_511_frames;   /* received frames with 256 ~ 511 bytes                                      */
        pPortCntrs->etherStatsRxPkts512to1023Octets      = data.rx_512_1023_frames;  /* received frames with 512 ~ 1023 bytes                                     */
        pPortCntrs->etherStatsRxPkts1024to1518Octets     = data.rx_1024_1518_frames; /* received frames with 1024 ~ 1518 bytes                                    */
        pPortCntrs->etherStatsRxPkts1519toMaxOctets      = data.rx_1519_max_frames;  /* received frames of which length is equal to or greater than 1519 bytes    */
        pPortCntrs->inOampduPkts                         = 0; //TBD waiting for Saturn Chip
        pPortCntrs->outOampduPkts                        = 0; //TBD waiting for Saturn Chip
    }
    return RT_ERR_OK;
}