/*
 * Copyright (C) 2012 Realtek Semiconductor Corp. 
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated, 
 * modified or distributed under the authorized license from Realtek. 
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER 
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED. 
 *
 * Purpose : For linux kernel mode
 *           MAC debug APIs in the SDK
 *
 * Feature : MAC debug APIs
 *
 */

/*
 * Include Files
 */
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <common/error.h>
#include <common/util.h>
#include <rtk/rtusr/rtusr_pkt.h>
#include <linux/slab.h>
#if !defined(CONFIG_LUNA_G3_SERIES)
#include "../../../../../../re8686_nic.h"
#endif

/*
 * Symbol Definition
 */

/*
 * Macro Declaration
 */
#define MAC_DEBUG_CONTINUOUSLY_TX_CNT       100

/*
 * Data Declaration
 */
static struct task_struct *pPktTxThread = NULL;
static uint8 *pktData = NULL;
static uint16 pktLen = 0;
static struct tx_info pktInfo;
static struct rx_info pktRxInfo;
static uint32 continueTxCnt = MAC_DEBUG_CONTINUOUSLY_TX_CNT;

/*
 * Function Declaration
 */
/* Copy debug tx info to actual NIC driver Tx info */
#if !defined(CONFIG_LUNA_G3_SERIES)
static int _mac_debug_dbgTxInfo2TxInfo(struct pkt_dbg_tx_info *pDbgTxInfo, struct tx_info *pTxInfo)
{
#if defined(CONFIG_RTL9607C_SERIES)
    GMAC_TXINFO_OWN(pTxInfo) = pDbgTxInfo->own;
    GMAC_TXINFO_EOR(pTxInfo) = pDbgTxInfo->eor;
    GMAC_TXINFO_FS(pTxInfo) = pDbgTxInfo->fs;
    GMAC_TXINFO_LS(pTxInfo) = pDbgTxInfo->ls;
    GMAC_TXINFO_IPCS(pTxInfo) = pDbgTxInfo->ipcs;
    GMAC_TXINFO_L4CS(pTxInfo) = pDbgTxInfo->l4cs;
    GMAC_TXINFO_TPID_SEL(pTxInfo) = pDbgTxInfo->tpid_sel;
    GMAC_TXINFO_STAG_AWARE(pTxInfo) = pDbgTxInfo->stag_aware;
    GMAC_TXINFO_CRC(pTxInfo) = pDbgTxInfo->crc;
    GMAC_TXINFO_DATA_LENGTH(pTxInfo) = pDbgTxInfo->data_length;

    GMAC_TXINFO_CPUTAG(pTxInfo) = pDbgTxInfo->cputag;
    GMAC_TXINFO_TX_SVLAN_ACTION(pTxInfo) = pDbgTxInfo->tx_svlan_action;
    GMAC_TXINFO_TX_CVLAN_ACTION(pTxInfo) = pDbgTxInfo->tx_cvlan_action;
    GMAC_TXINFO_TX_PMASK(pTxInfo) = pDbgTxInfo->tx_portmask;
    GMAC_TXINFO_CVLAN_VIDL(pTxInfo) = pDbgTxInfo->vidl;
    GMAC_TXINFO_CVLAN_PRIO(pTxInfo) = pDbgTxInfo->prio;
    GMAC_TXINFO_CVLAN_CFI(pTxInfo) = pDbgTxInfo->cfi;
    GMAC_TXINFO_CVLAN_VIDH(pTxInfo) = pDbgTxInfo->vidh;

    GMAC_TXINFO_ASPRI(pTxInfo) = pDbgTxInfo->aspri;
    GMAC_TXINFO_CPUTAG_PRI(pTxInfo) = pDbgTxInfo->cputag_pri;
    GMAC_TXINFO_KEEP(pTxInfo) = pDbgTxInfo->keep;
    GMAC_TXINFO_DISLRN(pTxInfo) = pDbgTxInfo->dislrn;
    GMAC_TXINFO_CPUTAG_PSEL(pTxInfo) = pDbgTxInfo->cputag_psel;
    GMAC_TXINFO_L34_KEEP(pTxInfo) = pDbgTxInfo->l34keep;
    GMAC_TXINFO_EXTSPA(pTxInfo) = pDbgTxInfo->extspa;
    GMAC_TXINFO_TX_PPPOE_ACTION(pTxInfo) = pDbgTxInfo->tx_pppoe_action;
    GMAC_TXINFO_TX_PPPOE_IDX(pTxInfo) = pDbgTxInfo->tx_pppoe_idx;
    GMAC_TXINFO_TX_DST_STREAM_ID(pTxInfo) = pDbgTxInfo->tx_dst_stream_id;

    GMAC_TXINFO_LGSEN(pTxInfo) = pDbgTxInfo->lgsen;
    GMAC_TXINFO_LGMTU(pTxInfo) = pDbgTxInfo->lgmtu;
    GMAC_TXINFO_SVLAN_VIDL(pTxInfo) = pDbgTxInfo->svlan_vidl;
    GMAC_TXINFO_SVLAN_PRIO(pTxInfo) = pDbgTxInfo->svlan_prio;
    GMAC_TXINFO_SVLAN_CFI(pTxInfo) = pDbgTxInfo->svlan_cfi;
    GMAC_TXINFO_SVLAN_VIDH(pTxInfo) = pDbgTxInfo->svlan_vidh;

    pTxInfo->opts3.bit.gmac_id = pDbgTxInfo->tx_gmac;
#else
    GMAC_TXINFO_OWN(pTxInfo) = pDbgTxInfo->own;
    GMAC_TXINFO_EOR(pTxInfo) = pDbgTxInfo->eor;
    GMAC_TXINFO_FS(pTxInfo) = pDbgTxInfo->fs;
    GMAC_TXINFO_LS(pTxInfo) = pDbgTxInfo->ls;
    GMAC_TXINFO_IPCS(pTxInfo) = pDbgTxInfo->ipcs;
    GMAC_TXINFO_L4CS(pTxInfo) = pDbgTxInfo->l4cs;
    GMAC_TXINFO_KEEP(pTxInfo) = pDbgTxInfo->keep;
    GMAC_TXINFO_BLU(pTxInfo) = pDbgTxInfo->blu;
    GMAC_TXINFO_CRC(pTxInfo) = pDbgTxInfo->crc;
    GMAC_TXINFO_VSEL(pTxInfo) = pDbgTxInfo->vsel;
    GMAC_TXINFO_DISLRN(pTxInfo) = pDbgTxInfo->dislrn;
    GMAC_TXINFO_CPUTAG_IPCS(pTxInfo) = pDbgTxInfo->ipcs;
    GMAC_TXINFO_CPUTAG_L4CS(pTxInfo) = pDbgTxInfo->l4cs;
    GMAC_TXINFO_CPUTAG_PSEL(pTxInfo) = pDbgTxInfo->cputag_psel;
    GMAC_TXINFO_DATA_LENGTH(pTxInfo) = pDbgTxInfo->data_length;

    GMAC_TXINFO_CPUTAG(pTxInfo) = pDbgTxInfo->cputag;
    GMAC_TXINFO_ASPRI(pTxInfo) = pDbgTxInfo->aspri;
    GMAC_TXINFO_CPUTAG_PRI(pTxInfo) = pDbgTxInfo->cputag_pri;
    GMAC_TXINFO_TX_VLAN_ACTION(pTxInfo) = pDbgTxInfo->tx_vlan_action;
    GMAC_TXINFO_TX_PPPOE_ACTION(pTxInfo) = pDbgTxInfo->tx_pppoe_action;
    GMAC_TXINFO_TX_PPPOE_IDX(pTxInfo) = pDbgTxInfo->tx_pppoe_idx;
    GMAC_TXINFO_EFID(pTxInfo) = pDbgTxInfo->efid;
    GMAC_TXINFO_ENHANCE_FID(pTxInfo) = pDbgTxInfo->enhance_fid;
    GMAC_TXINFO_VIDL(pTxInfo) = pDbgTxInfo->vidl;
    GMAC_TXINFO_PRIO(pTxInfo) = pDbgTxInfo->prio;
    GMAC_TXINFO_CFI(pTxInfo) = pDbgTxInfo->cfi;
    GMAC_TXINFO_VIDH(pTxInfo) = pDbgTxInfo->vidh;

    GMAC_TXINFO_EXTSPA(pTxInfo) = pDbgTxInfo->extspa;
    GMAC_TXINFO_TX_PMASK(pTxInfo) = pDbgTxInfo->tx_portmask;
    GMAC_TXINFO_TX_DST_STREAM_ID(pTxInfo) = pDbgTxInfo->tx_dst_stream_id;
#if defined(CONFIG_RTL9602C_SERIES)
    GMAC_TXINFO_SB(pTxInfo) = pDbgTxInfo->sb;
#endif
    GMAC_TXINFO_L34_KEEP(pTxInfo) = pDbgTxInfo->l34keep;
    GMAC_TXINFO_PTP(pTxInfo) = pDbgTxInfo->ptp;
#endif

    return 0;
}

/* Copy actual NIC driver Rx info to debug rx info */
static int _mac_debug_rxInfo2DbgRxInfo(struct rx_info *pRxInfo, struct pkt_dbg_rx_info *pDbgRxInfo)
{
    memset(pDbgRxInfo, 0, sizeof(struct pkt_dbg_rx_info));
#if defined(CONFIG_RTL9607C_SERIES)
    pDbgRxInfo->own = GMAC_RXINFO_OWN(pRxInfo);
    pDbgRxInfo->eor = GMAC_RXINFO_EOR(pRxInfo);
    pDbgRxInfo->fs = GMAC_RXINFO_FS(pRxInfo);
    pDbgRxInfo->ls = GMAC_RXINFO_LS(pRxInfo);
    pDbgRxInfo->crcerr = GMAC_RXINFO_CRCERR(pRxInfo);
    pDbgRxInfo->ipv4csf = GMAC_RXINFO_IPV4CSF(pRxInfo);
    pDbgRxInfo->l4csf = GMAC_RXINFO_L4CSF(pRxInfo);
    pDbgRxInfo->rcdf = GMAC_RXINFO_RCDF(pRxInfo);
    pDbgRxInfo->ipfrag = GMAC_RXINFO_IPFRAG(pRxInfo);
    pDbgRxInfo->pppoetag = GMAC_RXINFO_PPPOETAG(pRxInfo);
    pDbgRxInfo->rwt = GMAC_RXINFO_RWT(pRxInfo);
    pDbgRxInfo->data_length = GMAC_RXINFO_DATA_LENGTH(pRxInfo);

    pDbgRxInfo->cputag = GMAC_RXINFO_CPUTAG(pRxInfo);
    pDbgRxInfo->ptp_in_cpu_tag_exist = GMAC_RXINFO_PTP_EXIST(pRxInfo);
    pDbgRxInfo->svlan_exist = GMAC_RXINFO_SVLAN_EXIST(pRxInfo);
    pDbgRxInfo->reason = GMAC_RXINFO_REASON(pRxInfo);
    pDbgRxInfo->ctagva = GMAC_RXINFO_CTAGVA(pRxInfo);
    pDbgRxInfo->cvlan_tag = GMAC_RXINFO_CVLAN_TAG(pRxInfo);

    pDbgRxInfo->internal_priority = GMAC_RXINFO_INTERNAL_PRIORITY(pRxInfo);
    pDbgRxInfo->pon_stream_id = GMAC_RXINFO_PON_STREAM_ID(pRxInfo);
    pDbgRxInfo->pon_sid_or_extspa = GMAC_RXINFO_EXTSPA(pRxInfo);
    pDbgRxInfo->l3routing = GMAC_RXINFO_L3ROUTING(pRxInfo);
    pDbgRxInfo->origformat = GMAC_RXINFO_ORIGFORMAT(pRxInfo);
    pDbgRxInfo->src_port_num = GMAC_RXINFO_SRC_PORT_NUM(pRxInfo);
    pDbgRxInfo->fbi = GMAC_RXINFO_FBI(pRxInfo);
    pDbgRxInfo->dst_port_mask = GMAC_RXINFO_DST_PORT_MASK(pRxInfo);
    pDbgRxInfo->fb_hash = GMAC_RXINFO_FB_HASH(pRxInfo);
    /* MODIFY ME - dw & addr */
#else
    pDbgRxInfo->own = GMAC_RXINFO_OWN(pRxInfo);
    pDbgRxInfo->eor = GMAC_RXINFO_EOR(pRxInfo);
    pDbgRxInfo->fs = GMAC_RXINFO_FS(pRxInfo);
    pDbgRxInfo->ls = GMAC_RXINFO_LS(pRxInfo);
    pDbgRxInfo->crcerr = GMAC_RXINFO_CRCERR(pRxInfo);
    pDbgRxInfo->ipv4csf = GMAC_RXINFO_IPV4CSF(pRxInfo);
    pDbgRxInfo->pppoetag = GMAC_RXINFO_PPPOETAG(pRxInfo);
    pDbgRxInfo->rwt = GMAC_RXINFO_RWT(pRxInfo);
    pDbgRxInfo->pkttype = GMAC_RXINFO_PKTTYPE(pRxInfo);
    pDbgRxInfo->l3routing = GMAC_RXINFO_L3ROUTING(pRxInfo);
    pDbgRxInfo->origformat = GMAC_RXINFO_ORIGFORMAT(pRxInfo);
    pDbgRxInfo->pctrl = GMAC_RXINFO_PCTRL(pRxInfo);
    pDbgRxInfo->data_length = GMAC_RXINFO_DATA_LENGTH(pRxInfo);

    pDbgRxInfo->cputag = GMAC_RXINFO_CPUTAG(pRxInfo);
    pDbgRxInfo->ptp_in_cpu_tag_exist = GMAC_RXINFO_PTP_EXIST(pRxInfo);
    pDbgRxInfo->svlan_exist = GMAC_RXINFO_SVLAN_EXIST(pRxInfo);
    pDbgRxInfo->pon_stream_id = GMAC_RXINFO_PON_STREAM_ID(pRxInfo);
    pDbgRxInfo->ctagva = GMAC_RXINFO_CTAGVA(pRxInfo);
    pDbgRxInfo->cvlan_tag = GMAC_RXINFO_CVLAN_TAG(pRxInfo);

    pDbgRxInfo->src_port_num = GMAC_RXINFO_SRC_PORT_NUM(pRxInfo);
    pDbgRxInfo->dst_port_mask = GMAC_RXINFO_DST_PORT_MASK(pRxInfo);
    pDbgRxInfo->reason = GMAC_RXINFO_REASON(pRxInfo);
    pDbgRxInfo->internal_priority = GMAC_RXINFO_INTERNAL_PRIORITY(pRxInfo);
    pDbgRxInfo->ext_port_ttl_1 = GMAC_RXINFO_EXT_PORT_TTL_1(pRxInfo);
    /* MODIFY ME - dw & addr */
#endif
    return 0;
}
#endif

int mac_debug_pktTx_thread(void *data)
{
#if !defined(CONFIG_LUNA_G3_SERIES)
#if !defined(CONFIG_LUNA_DBG_REMOVE_NIC)
    int i;
    int re8686_tx_with_Info_dbg(unsigned char *pPayload, unsigned short length, void *pInfo);

    while(!kthread_should_stop())
    {
        /* It is possible to wake up earlier */
        set_current_state(TASK_INTERRUPTIBLE);
        schedule_timeout(1 * HZ / 1000);

        pktInfo.opts2.bit.cputag = 1;
        if(likely(pktData != NULL))
        {
            for(i = 0;i < continueTxCnt;i++)
            {
                re8686_tx_with_Info_dbg(pktData, pktLen, &pktInfo);
            }
        }
    }
#endif
#endif
    return 0;
}

/* Function Name:
 *      mac_debug_rxPkt_enable
 * Description:
 *      Due to the rx packet debug might impact the performance, this API is to
 *      enable/disable the rx packet debug function
 * Input:
 *      enable - enable or disable the rx packet storage
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 * Note:
 *      None
 */
 int32 
 mac_debug_rxPkt_enable(uint32 enable)
{
#if !defined(CONFIG_LUNA_G3_SERIES)
#if !defined(CONFIG_LUNA_DBG_REMOVE_NIC)
#if defined(CONFIG_SOC_DEPEND_FEATURE)
    int re8686_rx_pktDump_set(unsigned int enable);
    
    re8686_rx_pktDump_set(enable);
#endif
#endif
#endif
    return RT_ERR_OK;
}

/* Function Name:
 *      mac_debug_rxPkt_get
 * Description:
 *      Get the last received packet
 * Input:
 *      pPayload   - entire packet to be sent
 *      buf_length - specified the maximum length to be dumped
 *      pkt_length - get the actul length of packet
 *      pInfo      - rx descriptor of packet
 *      pEnable    - get the dump copy state
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 * Note:
 *      Dump command always success
 */
int32 
mac_debug_rxPkt_get(uint8 *pPayload, uint16 buf_length, uint16 *pPkt_len, struct pkt_dbg_rx_info *pInfo, unsigned int *pEnable)
{
#if !defined(CONFIG_LUNA_G3_SERIES)
#if !defined(CONFIG_LUNA_DBG_REMOVE_NIC)
#if defined(CONFIG_SOC_DEPEND_FEATURE)
    int re8686_rx_pktDump_get(unsigned char *pPayload, unsigned short buf_length, unsigned short *pPkt_len, void *pInfo, unsigned int *pEnable);

    re8686_rx_pktDump_get(pPayload, buf_length, pPkt_len, &pktRxInfo, pEnable);
    _mac_debug_rxInfo2DbgRxInfo(&pktRxInfo, pInfo);
#endif
#endif
#endif
    return RT_ERR_OK;
}

/* Function Name:
 *      mac_debug_rxPktFifo_get
 * Description:
 *      Get the last received packet
 * Input:
 *      fifo_idx   - FIFO index to be dumped
 *      pPayload   - entire packet to be sent
 *      buf_length - specified the maximum length to be dumped
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 * Note:
 */
int32 
mac_debug_rxPktFifo_get(uint16 fifo_idx, uint8 *pPayload, uint16 buf_length)
{
#if !defined(CONFIG_LUNA_G3_SERIES)
#if !defined(CONFIG_LUNA_DBG_REMOVE_NIC)
#if defined(CONFIG_SOC_DEPEND_FEATURE)
    int re8686_rx_pktFifoDump_get(unsigned int fifo_idx, unsigned char *pPayload, unsigned short buf_length);

    return re8686_rx_pktFifoDump_get(fifo_idx, pPayload, buf_length);
#endif
#endif
#endif
}

/* Function Name:
 *      mac_debug_rxPkt_clear
 * Description:
 *      Clear the last received packet storage buffer
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 * Note:
 *      Clear command always success
 */
int32 
mac_debug_rxPkt_clear(void)
{
#if !defined(CONFIG_LUNA_G3_SERIES)
#if !defined(CONFIG_LUNA_DBG_REMOVE_NIC)
#if defined(CONFIG_SOC_DEPEND_FEATURE)
    int re8686_rx_pktDump_clear(void);
    
    re8686_rx_pktDump_clear();
#endif
#endif
#endif
    return RT_ERR_OK;
}

/* Function Name:
 *      mac_debug_txPkt_send
 * Description:
 *      Send the packet in the buffer
 * Input:
 *      pPayload - entire packet to be sent
 *      length   - packet length
 *      pInfo    - tx descriptor of packet
 *      count    - tx repeat count
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 * Note:
 *      Send command always success
 */
int32 
mac_debug_txPkt_send(uint8 *pPayload, uint16 length, struct pkt_dbg_tx_info *pInfo, uint32 count)
{
#if !defined(CONFIG_LUNA_G3_SERIES)
#if !defined(CONFIG_LUNA_DBG_REMOVE_NIC)
#if defined(CONFIG_SOC_DEPEND_FEATURE)
    int i;
    int re8686_tx_with_Info_dbg(unsigned char *pPayload, unsigned short length, void *pInfo);

    _mac_debug_dbgTxInfo2TxInfo(pInfo, &pktInfo);
    GMAC_TXINFO_CPUTAG(&pktInfo) = 1;
    for(i = 0; i < count; i ++)
    {
        re8686_tx_with_Info_dbg(pPayload, length, &pktInfo);
    }
#endif
#endif
#endif
    return RT_ERR_OK;
}

/* Function Name:
 *      mac_debug_txPktCont_send
 * Description:
 *      Send the packet in the buffer continuously
 * Input:
 *      pPayload - entire packet to be sent
 *      length   - packet length
 *      pInfo    - tx descriptor of packet
 *      enable   - enable/disable continous sending
 * Output:
 *      None
 * Return:
 *      Continue send command always success
 *      RT_ERR_OK
 */
int32 
mac_debug_txPktCont_send(uint8 *pPayload, uint16 length, struct pkt_dbg_tx_info *pInfo, uint8 enable)
{
#if !defined(CONFIG_LUNA_G3_SERIES)
#if defined(CONFIG_SOC_DEPEND_FEATURE)
    static uint8 state = 0;

    if(enable == state)
    {
        /* Woops? The same command twice? */
    }
    else
    {
        if(enable != 0)
        {
            /* 0 -> non-zero: start continuously packet xmit */
            if(pPktTxThread == NULL)
            {
                pPktTxThread = kthread_create(mac_debug_pktTx_thread, NULL, "dbgPktContTx");
            }

            if(IS_ERR(pPktTxThread))
            {
                printk("%s:%d mac_debug_pktTx_thread failed %ld!\n", __FILE__, __LINE__, PTR_ERR(pPktTxThread));
                return RT_ERR_OK;
            }
            else
            {
                if(pktData != NULL)
                {
                    kfree(pktData);
                }
                /* Thread create success, copy the necessary data for thread running */
                pktData = (uint8 *)kmalloc(sizeof(uint8) * length, GFP_KERNEL);
                /* If the malloc failed, NULL pointer will cause thread not sending anything */
                if(pktData)
                {
                    memcpy(pktData, pPayload, sizeof(uint8) * length);
                    pktLen = length;
                    _mac_debug_dbgTxInfo2TxInfo(pInfo, &pktInfo);
                    wake_up_process(pPktTxThread);
                }
                else
                {
                    /* Leave the created thread for next run and don't update the state */
                    return RT_ERR_OK;
                }
            }
        }
        else
        {
            /* non-zero -> 0: stop continuously packet xmit */
            if(pPktTxThread)
            {
                kthread_stop(pPktTxThread);
                pPktTxThread = NULL;
            }
            if(pktData)
            {
                kfree(pktData);
                pktData = NULL;
            }
        }

        /* state update */
        state = enable;
    }

#endif
#endif
    return RT_ERR_OK;
}

/* Function Name:
 *      mac_debug_txPktContCnt_set
 * Description:
 *      Set send count for each time sending thread wake up
 * Input:
 *      count    - tx repeat count
 * Output:
 *      None
 * Return:
 *      RT_ERR_OK
 */
int32 
mac_debug_txPktContCnt_set(uint32 count)
{
    continueTxCnt = count;
    return RT_ERR_OK;
}

/* Function Name:
 *      mac_debug_txPktContCnt_get
 * Description:
 *      Get send count for each time sending thread wake up
 * Input:
 *      None
 * Output:
 *      pCount    - tx repeat count
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_INPUT
 */
int32 
mac_debug_txPktContCnt_get(uint32 *pCount)
{
    if(pCount != NULL)
    {
        *pCount = continueTxCnt;
        return RT_ERR_OK;
    }
    return RT_ERR_INPUT;
}

#if defined(CONFIG_SDK_RTL9607C)
/* Function Name:
 *      mac_debug_rxGmac_set
 * Description:
 *      Set debug rx dump from what gmac num
 * Input:
 *      gmac     - rx gmac number
 * Output:
 *      
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_INPUT
 */
int32 
mac_debug_rxGmac_set(uint32 gmac)
{
#if defined(CONFIG_SOC_DEPEND_FEATURE)
#if !defined(CONFIG_LUNA_DBG_REMOVE_NIC)
    int re8686_rx_gmac_set(unsigned int gmac);

    re8686_rx_gmac_set(gmac);
#endif
#endif
    return RT_ERR_INPUT;
}
/* Function Name:
 *      mac_debug_rxGmac_get
 * Description:
 *      Get debug rx dump from what gmac num
 * Input:
 *      
 * Output:
 *      pGmac     - rx gmac number
 * Return:
 *      RT_ERR_OK
 *      RT_ERR_INPUT
 */
int32 
mac_debug_rxGmac_get(uint32 *pGmac)
{
#if defined(CONFIG_SOC_DEPEND_FEATURE)
#if !defined(CONFIG_LUNA_DBG_REMOVE_NIC)
    int re8686_rx_gmac_get(unsigned int *pGmac);

    re8686_rx_gmac_get(pGmac);
#endif
#endif
    return RT_ERR_INPUT;
}
#endif
