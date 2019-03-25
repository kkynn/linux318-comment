/*
 * Copyright (C) 2018 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 *
 *
 * $Revision: $
 * $Date: $
 *
 * Purpose : EPON kernel drivers
 *
 * Feature : This module install EPON kernel callbacks and other 
 * kernel functions
 *
 */

#ifdef CONFIG_CA_RTK_EPON_FEATURE
/*
 * Include Files
 */
#include <linux/module.h> 
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <common/error.h>
#include <common/rt_type.h>
#include <common/rt_error.h>

#include "pkt_redirect.h"

#include "hal/common/halctrl.h"

#include "ni-drv/ca_ni.h"
#include "ni-drv/ca_ext.h"
#include "cortina-api/include/pkt_tx.h"
#include "cortina-api/include/port.h"
#include "cortina-api/include/special_packet.h"
#include "cortina-api/include/epon.h"
#include "cortina-api/include/classifier.h"
#include "cortina-api/include/vlan.h"
#include "aal/include/aal_epon.h"
#include "aal/include/aal_mpcp.h"
#include "aal/include/aal_puc.h"
#include "aal/include/aal_psds.h"
#include "scfg/include/scfg.h"
#include "ca_rtk_epon_drv_main.h"

#include <ca_event.h>

#include "rtk/epon.h"
#include "rtk/l2.h"

extern netdev_tx_t nic_egress_start_xmit(struct sk_buff *skb, struct net_device *dev, ca_ni_tx_config_t *tx_config);
extern int drv_nic_register_rxhook(int portmask,int priority,p2rfunc_t rx);
extern int drv_nic_unregister_rxhook(int portmask,int priority,p2rfunc_t rx);

static ca_uint32_t gbl_cls_index=-1;

#define MAX_QUEUE 128
#define MAX_LLID 1
#define MAX_COS 8
static ca_uint32_t gbl_us_pon_cls_index[MAX_QUEUE];

#define ETYPE_OFFSET    12
#define ETH_HEAD_LEN    16

//reference to ca_ni_tx.c
static ca_mac_addr_t puc_da = {0x00, 0x13, 0x25, 0x00, 0x00, 0x00};
static ca_mac_addr_t puc_sa = {0x00, 0x13, 0x25, 0x00, 0x00, 0x01};

static ca_int8_t etype_oam[] = {0xff, 0xf1};

static ca_mac_addr_t oam_da = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x02};

static uint8 one_shot_setting = 0;
static uint8 oam_static_l2_addr_setting = 0;

static int saturn_us_pon_rule_adding(void);
static int saturn_us_pon_rule_deleting(void);

int oam_ca_drv_log = 0;

#define OAM_CA_DRV_LOG(level,fmt, arg...) \
    do { if ( oam_ca_drv_log > level) { printk(fmt, ##arg); printk("\n"); } } while (0);

struct proc_dir_entry* ca_rtk_epon_proc_dir = NULL;
struct proc_dir_entry* oam_ca_drv_log_entry = NULL;
struct proc_dir_entry* mpcp_stats_entry = NULL;
struct proc_dir_entry* port_stats_entry = NULL;
struct proc_dir_entry* us_pon_rule_entry = NULL;
struct proc_dir_entry* oam_static_l2_addr_entry = NULL;
struct proc_dir_entry* mpcp_reg_start_entry = NULL;
struct proc_dir_entry* pon_mac_reset_entry = NULL;


static void oam_static_l2_addr_add(void)
{
    int32 ret;
    rtk_l2_ucastAddr_t l2Addr;

    //Adding L2 mac(00:13:25:00:00:00) at port 7 for guaranting OAM packet can be traped at cos 7 when sw learning is enabling 
    memset(&l2Addr, 0x00, sizeof(rtk_l2_ucastAddr_t));  
    l2Addr.vid = 0;
    memcpy(&l2Addr.mac.octet, puc_da, ETHER_ADDR_LEN);
    ret = rtk_l2_addr_get(&l2Addr);
    if(ret == RT_ERR_L2_ENTRY_NOTFOUND)
    {
        memset(&l2Addr, 0x00, sizeof(rtk_l2_ucastAddr_t));
        l2Addr.flags |= RTK_L2_UCAST_FLAG_IVL|RTK_L2_UCAST_FLAG_STATIC;
        l2Addr.vid = 0;
        memcpy(&l2Addr.mac.octet, puc_da, ETHER_ADDR_LEN);
        l2Addr.port = HAL_GET_PON_PORT();

        oam_static_l2_addr_setting = 1;
        OAM_CA_DRV_LOG(0,"Adding deault PUC address (%02x:%02x:%02x:%02x:%02x:%02x)!!\n",puc_da[0],puc_da[1],puc_da[2],puc_da[3],puc_da[4],puc_da[5]);
        if((ret = rtk_l2_addr_add(&l2Addr))!=CA_E_OK)
        {
            oam_static_l2_addr_setting = 0;
            OAM_CA_DRV_LOG(0,"Adding deault PUC address (%02x:%02x:%02x:%02x:%02x:%02x) Error ret=%d!!\n",puc_da[0],puc_da[1],puc_da[2],puc_da[3],puc_da[4],puc_da[5],ret);
        }
    }
}

static void oam_static_l2_addr_del(void)
{
    int32 ret;
    rtk_l2_ucastAddr_t l2Addr;

    //Adding L2 mac(00:13:25:00:00:00) at port 7 for guaranting OAM packet can be traped at cos 7 when sw learning is enabling 
    memset(&l2Addr, 0x00, sizeof(rtk_l2_ucastAddr_t));  
    l2Addr.vid = 0;
    memcpy(&l2Addr.mac.octet, puc_da, ETHER_ADDR_LEN);
    ret = rtk_l2_addr_get(&l2Addr);
    if(ret == RT_ERR_OK)
    {
        oam_static_l2_addr_setting = 0;
        OAM_CA_DRV_LOG(0,"Deleting deault PUC address (%02x:%02x:%02x:%02x:%02x:%02x)!!\n",puc_da[0],puc_da[1],puc_da[2],puc_da[3],puc_da[4],puc_da[5]);
        if((ret = rtk_l2_addr_del(&l2Addr))!=CA_E_OK)
        {
            oam_static_l2_addr_setting = 1;
            OAM_CA_DRV_LOG(0,"Deleting deault PUC address (%02x:%02x:%02x:%02x:%02x:%02x) Error ret=%d!!\n",puc_da[0],puc_da[1],puc_da[2],puc_da[3],puc_da[4],puc_da[5],ret);
        }
    }
}

s_oam_t S_OAM={
    .num = 0,
};

void s_oam_register(s_oam_t *reg_oam)
{
    if(reg_oam->num > MAX_S_OAM_NUM)
    {
        printk("%s %d illegal register oam number=%d\n",__FUNCTION__,__LINE__,reg_oam->num);
        return;
    }
    printk("%s %d register oam number=%d\n",__FUNCTION__,__LINE__,reg_oam->num);
    memcpy(&S_OAM,reg_oam,sizeof(s_oam_t));

    oam_ca_drv_log = 3;
}

void s_oam_unregister(void)
{
    memset(&S_OAM,0,sizeof(s_oam_t));

    oam_ca_drv_log = 0;
}

void send_static_oam_pkt_tx(unsigned char llidIdx,unsigned short dataLen, unsigned char *data)
{
    ca_pkt_t halPkt;
    ca_pkt_block_t pkt_data;
    unsigned char tmp[60];
    int ret;

    memset(&halPkt,0,sizeof(halPkt));
    halPkt.block_count    = 1;
    halPkt.cos            = 8;
    halPkt.device_id      = 0;
    halPkt.src_port_id    = CA_PORT_ID(CA_PORT_TYPE_CPU, 0x10);
    halPkt.dst_port_id    = CA_PORT_ID(CA_PORT_TYPE_EPON, 7);
    halPkt.pkt_type       = CA_PKT_TYPE_OAM;
    halPkt.pkt_len        = dataLen;
    halPkt.pkt_data       = (ca_pkt_block_t *)&pkt_data;
    halPkt.pkt_data->len  = dataLen;
    halPkt.pkt_data->data = data;
    halPkt.flags          = CA_TX_BYPASS_FWD_ENGINE;
    halPkt.dst_sub_port_id = llidIdx;

    if(dataLen < 60)
    {
        memset(tmp,0,60);
        memcpy(tmp,data,dataLen);
        halPkt.pkt_len        = 60;
        halPkt.pkt_data->len  = 60;
        halPkt.pkt_data->data = tmp;
    }

    if ((ret = __ca_tx(0, &halPkt)) != CA_E_OK)
    {
        OAM_CA_DRV_LOG(2,"%s %d : TX OAM Error!!\n",__FUNCTION__,__LINE__);
    }

    OAM_CA_DRV_LOG(2,"======%s %d : TX OAM len %u ======\n",__FUNCTION__,__LINE__,dataLen);
    OAM_CA_DRV_LOG(2,"%s %d : TX OAM content\n",__FUNCTION__,__LINE__);
    if (oam_ca_drv_log > 2)
        print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 1, data, dataLen, true);
}

void ca_rtk_epon_oam_pkt_tx(unsigned short dataLen, unsigned char *data)
{
    ca_pkt_t halPkt;
    ca_pkt_block_t pkt_data;
    int ret;
    unsigned char llidIdx;
    unsigned char tmp[60];

    llidIdx = *((unsigned char *) (data + (dataLen - sizeof(unsigned char))));
    dataLen = dataLen - sizeof(unsigned char); /*forllid*/

    memset(&halPkt,0,sizeof(halPkt));
    halPkt.block_count    = 1;
    halPkt.cos            = 8;
    halPkt.device_id      = 0;
    halPkt.src_port_id    = CA_PORT_ID(CA_PORT_TYPE_CPU, 0x10);
    halPkt.dst_port_id    = CA_PORT_ID(CA_PORT_TYPE_EPON, 7);
    halPkt.pkt_type       = CA_PKT_TYPE_OAM;
    halPkt.pkt_len        = dataLen;
    halPkt.pkt_data       = (ca_pkt_block_t *)&pkt_data;
    halPkt.pkt_data->len  = dataLen;
    halPkt.pkt_data->data = data;
    halPkt.flags          = CA_TX_BYPASS_FWD_ENGINE;
    halPkt.dst_sub_port_id = llidIdx;

    if(dataLen < 60)
    {
        memset(tmp,0,60);
        memcpy(tmp,data,dataLen);
        halPkt.pkt_len        = 60;
        halPkt.pkt_data->len  = 60;
        halPkt.pkt_data->data = tmp;
    }

    OAM_CA_DRV_LOG(2,"======%s %d : TX OAM len %u ======\n",__FUNCTION__,__LINE__,dataLen);
    OAM_CA_DRV_LOG(2,"%s %d : TX OAM content\n",__FUNCTION__,__LINE__);
    if (oam_ca_drv_log > 2)
        print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 1, data, dataLen, true);

    if(memcmp(data,oam_da,6))
    {
        OAM_CA_DRV_LOG(2,"%s %d : Not OAM DA no send\n",__FUNCTION__,__LINE__);
        return ;
    }

    OAM_CA_DRV_LOG(2,"%s %d : __ca_tx send OAM \n",__FUNCTION__,__LINE__);
    if ((ret = __ca_tx(0, &halPkt)) != CA_E_OK)
    {
        OAM_CA_DRV_LOG(2,"%s %d : TX OAN Error!!\n",__FUNCTION__,__LINE__);
    }
}

void ca_rtk_epon_oam_dyingGasp_tx(unsigned short dataLen, unsigned char *data)
{
    ca_uint32_t ret = CA_E_OK;

    if((ret = aal_epon_dying_gasp_set(0,0,data,dataLen)) != CA_E_OK)
    {
        OAM_CA_DRV_LOG(0,"%s %d : aal_epon_dying_gasp_set Error!!",__FUNCTION__,__LINE__);
        return ;
    }
}

int ca_rtk_epon_oam_pkt_rx(struct napi_struct *napi,struct net_device *dev, struct sk_buff *skb, nic_hook_private_t *nh_priv)
{
    int ret;
    uint8_t * oam_ptr;
    uint32_t oam_len;
    unsigned char llidIdx; 

    oam_ptr = skb->data+ETH_HEAD_LEN;
    oam_len = skb->len-ETH_HEAD_LEN;

    //check oam ether type
    if (0 == memcmp(&skb->data[12], etype_oam, sizeof(etype_oam)))
    {
        llidIdx = skb->data[11] & 0x3f;  /* restore llid: puc_da[5:0] */

        OAM_CA_DRV_LOG(2,"======%s %d : RX skb len %u llidIdx %u======\n",__FUNCTION__,__LINE__,skb->len,llidIdx);
        OAM_CA_DRV_LOG(2,"%s %d : RX OAM pre header\n",__FUNCTION__,__LINE__);
        if (oam_ca_drv_log > 2)
            print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 1, skb->data, ETH_HEAD_LEN, true);

        OAM_CA_DRV_LOG(2,"%s %d : RX OAM content\n",__FUNCTION__,__LINE__);
        if (oam_ca_drv_log > 2)
            print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 1, oam_ptr, oam_len, true);

        if((oam_ptr[12] == 0x88) && (oam_ptr[13] == 0x09) && (oam_ptr[14] == 0x03))
        {
            /* llid index of boardcast oam packet is 0x08, 0x0A(10) or 0x0B(11), it must be changed to llid 0 */
            if(llidIdx == 0x08 || llidIdx == 0x0A || llidIdx == 0x0B)
                llidIdx = 0;

            if(S_OAM.num > 0)
            {
                int i,j,match;
                ca_mac_addr_t olt_mac;
                ca_mac_addr_t onu_mac;

                match = 0;

                memset((void *)&olt_mac[0], 0, sizeof(olt_mac));
                aal_mpcp_olt_mac_addr_get(0, 0, olt_mac);

                for(j=0;j<S_OAM.num;j++)
                {
                    match = 1;
                    for(i=12;i<S_OAM.rx_len[j]-12;i++)
                    {
                        if(oam_ptr[i]!=S_OAM.rx[j][i])
                        {
                            match = 0;
                            break;
                        }
                    }
                    if(match == 1)
                        break;
                }
                
                if(match == 1)
                {
                    memset((void *)&onu_mac[0], 0, sizeof(onu_mac));
                    aal_mpcp_mac_addr_get(0, 0, llidIdx, onu_mac);
                    memcpy(&S_OAM.tx[j][6],&onu_mac[0],6);

                    OAM_CA_DRV_LOG(2,"OAM entry num = %d\n",j+1);
                    send_static_oam_pkt_tx(llidIdx,S_OAM.tx_len[j],S_OAM.tx[j]);
                }
                else
                {
                    OAM_CA_DRV_LOG(2,"====Not Match RX OAM content oam_len=%d====\n",oam_len);
                    if (oam_ca_drv_log > 2)
                        print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 1, oam_ptr, oam_len, true);
                }
                return RE8670_RX_STOP;
            }

            skb_put(skb, sizeof(llidIdx));
            *((unsigned char *)(skb->data + (skb->len - sizeof(llidIdx)))) = llidIdx;
            
            ret = pkt_redirect_kernelApp_sendPkt(PR_USER_UID_EPONOAM, 1, oam_len +  sizeof(llidIdx), oam_ptr);
            if(ret)
            {
                OAM_CA_DRV_LOG(2,"send to user app (%d) fail (%d)\n", PR_USER_UID_EPONOAM, ret);
            }

            return RE8670_RX_STOP;
        }
    }
    return RE8670_RX_CONTINUE;
}

ca_uint32_t ca_rtk_epon_reg_intr(ca_device_id_t device_id,ca_uint32_t event_id,ca_void *event,ca_uint32_t len,ca_void *cookie)
{
    ca_event_reg_stat_t* evt_ptr = (ca_event_reg_stat_t *)event;

    if(evt_ptr->status == 1)
    {
        if(one_shot_setting == 0)
        {
            oam_static_l2_addr_add();
            saturn_us_pon_rule_adding();
            one_shot_setting = 1;
        }
        OAM_CA_DRV_LOG(0,"llid-idx %d llid %d register\n",evt_ptr->llid_idx,evt_ptr->llid_value);
    }
    else
    {
        OAM_CA_DRV_LOG(0,"llid-idx %d llid %d de-register\n",evt_ptr->llid_idx,evt_ptr->llid_value);
    }
}

ca_uint32_t ca_rtk_epon_port_intr(ca_device_id_t device_id,ca_uint32_t event_id,ca_void *event,ca_uint32_t len,ca_void *cookie)
{
    ca_event_epon_port_link_t* evt_ptr = (ca_event_epon_port_link_t *)event;

    unsigned char losMsg[5] = {
        0x15, 0x68, /* Magic key */
        0xde, 0xad  /* Message body */
    };

    unsigned char losRecoverMsg[5] = {
        0x15, 0x68, /* Magic key */
        0x55, 0x55  /* Message body */
    };

    if(evt_ptr->port_id == CA_PORT_ID(CA_PORT_TYPE_EPON, CA_PORT_ID_NI7))
    {
        if(evt_ptr->status == 1)
        {
            OAM_CA_DRV_LOG(0,"epon port link up\n");
            pkt_redirect_kernelApp_sendPkt(PR_USER_UID_EPONOAM, 1, sizeof(losRecoverMsg), losRecoverMsg);
        }
        else
        {
            OAM_CA_DRV_LOG(0,"epon port link down\n");
            pkt_redirect_kernelApp_sendPkt(PR_USER_UID_EPONOAM, 1, sizeof(losMsg), losMsg);
        }
    }
}

ca_uint32_t ca_rtk_epon_silence_intr(ca_device_id_t device_id,ca_uint32_t event_id,ca_void *event,ca_uint32_t len,ca_void *cookie)
{
    ca_event_silence_stat_t* evt_ptr = (ca_event_silence_stat_t *)event;

    unsigned char silentMsg[] = {
        0x15, 0x68, /* Magic key */
        0x66, 0x66  /* Message body */
    };

    printk("llid-idx %d silence status=%d\n",evt_ptr->llid_idx,evt_ptr->status);

    OAM_CA_DRV_LOG(0,"llid-idx %d silence status=%d\n",evt_ptr->llid_idx,evt_ptr->status);

    if(evt_ptr->status == 1)
    {
        pkt_redirect_kernelApp_sendPkt(PR_USER_UID_EPONOAM, 1, sizeof(silentMsg), silentMsg);
    }
}

static int saturn_us_pon_vlan_port_add(unsigned int vid,ca_port_id_t port)
{
    unsigned char count = 1;
    ca_uint8_t oriCount = 0;
    ca_uint8_t oriUntagCount = 0;
    ca_uint8_t idx = 0;
    ca_status_t ret = CA_E_OK;
    ca_uint8_t wanFlag = FALSE;
    ca_port_id_t memPort[9];
    ca_port_id_t oriMemPort[9];
    ca_port_id_t oriUntagMemPort[9];

    ret = ca_l2_vlan_create(0, vid);
    if ((CA_E_OK != ret) && (CA_E_EXISTS != ret))
    {
        printk("%s %d vlan create fail vid:%d ret:%d",
            __FUNCTION__, __LINE__, vid, ret);
        return RT_ERR_FAILED;
    }

    if (CA_E_OK != ca_l2_vlan_port_get(0,
                                       vid,
                                       &oriCount,
                                       oriMemPort,
                                       &oriUntagCount,
                                       oriUntagMemPort))
    {
        printk("%s %d ca_l2_vlan_port_get failed",__FUNCTION__, __LINE__);
        return RT_ERR_FAILED;
    }

    for (idx = 0; idx < oriCount; idx++)
    {
        if (oriMemPort[idx] == port)
        {
            return RT_ERR_OK;
        }
        if ((oriMemPort[idx] == 7)
          || (port == CA_PORT_ID(CA_PORT_TYPE_EPON,CA_PORT_ID_NI7)))
        {
            wanFlag = TRUE;
        }
    }

    //if no wan configure, configure wan port too!
    if (FALSE == wanFlag)
    {
        count++;
        memPort[oriCount + 1] = CA_PORT_ID(CA_PORT_TYPE_EPON,CA_PORT_ID_NI7);
    }

    if ((9 <= oriCount) || (9 < (oriCount + count)))
    {
        printk("%s %d add 9 port in this vlan[%d] now",__FUNCTION__, __LINE__, vid);
        return RT_ERR_OK;
    }

    memcpy(memPort, oriMemPort, sizeof(ca_port_id_t) * oriCount);
    count  = count + oriCount;
    memPort[oriCount] = port;

    printk("vid   [%d]", vid);
    printk("count [%d]", count);
    for (idx = 0; idx < count; idx++)
    {
        printk("memPort[%d].port[0x%x]", idx, memPort[idx]);
    }
    printk("untag_count [%d]", oriUntagCount);
    for (idx = 0; idx < count; idx++)
    {
        printk("untag_memPort[%d].port[0x%x]", idx, oriUntagMemPort[idx]);
    }

    ret = ca_l2_vlan_port_set(0, vid, count, memPort, oriUntagCount, oriUntagMemPort);
    if (CA_E_OK != ret)
    {
        printk("%s %d ca_l2_vlan_port_set failed ret[%d]",__FUNCTION__, __LINE__, ret);
        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

static int saturn_us_pon_rule_setting(ca_uint32_t llid,ca_uint32_t cos,ca_uint32_t *cls_index,ca_uint8_t flag)
{
    int32_t ret = RT_ERR_OK;
    ca_uint32_t priority;
    ca_classifier_key_t key;
    ca_classifier_key_mask_t key_mask;
    ca_classifier_action_t action;
    ca_uint32_t get_priority;
    ca_classifier_key_t get_key;
    ca_classifier_key_mask_t get_key_mask;
    ca_classifier_action_t get_action;
    ca_uint32_t rtk_llid_cos,ca_llid_cos;

    rtk_llid_cos = (llid<<4) | cos;
    ca_llid_cos = (llid<<11) | cos<<8;

    memset(&key,0,sizeof(ca_classifier_key_t));
    memset(&key_mask,0,sizeof(ca_classifier_key_mask_t));
    memset(&action,0,sizeof(ca_classifier_action_t));

    priority=7;

    key.src_port = CA_PORT_ID(CA_PORT_TYPE_ETHERNET, 6);
    key_mask.src_port = 1;

    key.l2.vlan_count = 1;
    key_mask.l2 = 1;

    key.l2.vlan_otag.vlan_max.vid = rtk_llid_cos;
    key.l2.vlan_otag.vlan_min.vid = rtk_llid_cos;
    key_mask.l2_mask.vlan_otag = 1;
    key_mask.l2_mask.vlan_otag_mask.vid =1;

    action.forward = CA_CLASSIFIER_FORWARD_PORT;
    action.dest.port = CA_PORT_ID(CA_PORT_TYPE_EPON, 7);

    action.options.action_handle.llid_cos_index = ca_llid_cos;
    action.options.masks.action_handle = 1;

    action.options.outer_vlan_act = CA_CLASSIFIER_VLAN_ACTION_POP;
    action.options.masks.outer_vlan_act = 1;

    if(flag == 1) //Adding
    {
        OAM_CA_DRV_LOG(1,"%s %d : saturn_vlan_port_add!!\n",__FUNCTION__,__LINE__);
        if((ret = saturn_us_pon_vlan_port_add(rtk_llid_cos,CA_PORT_ID(CA_PORT_TYPE_ETHERNET,CA_PORT_ID_NI6))) != CA_E_OK)
        {
            printk("%s %d : saturn_vlan_port_add Error ret=%d!!\n",__FUNCTION__,__LINE__,ret);
            return RT_ERR_FAILED;
        }
        OAM_CA_DRV_LOG(1,"%s %d : saturn_vlan_port_add Success!!\n",__FUNCTION__,__LINE__);

        OAM_CA_DRV_LOG(1,"%s %d : ca_classifier_rule_add cls_index=%d!!\n",__FUNCTION__,__LINE__,*cls_index);
        /*CA Driver support check existing rule if existing will return correct rule index before adding rule*/
        if((ret = ca_classifier_rule_add(0,priority,&key,&key_mask,&action,cls_index)) != CA_E_OK)
        {
            printk("%s %d : ca_classifier_rule_add Error ret=%d!!\n",__FUNCTION__,__LINE__,ret);
            return RT_ERR_FAILED;
        }
        OAM_CA_DRV_LOG(1,"%s %d : ca_classifier_rule_add Success cls_index=%d!!\n",__FUNCTION__,__LINE__,*cls_index);
    }
    else //Deleting
    {
        memset(&get_key,0,sizeof(ca_classifier_key_t));
        memset(&get_key_mask,0,sizeof(ca_classifier_key_mask_t));
        memset(&get_action,0,sizeof(ca_classifier_action_t));

        OAM_CA_DRV_LOG(1,"%s %d : ca_classifier_rule_get cls_index=%d!!\n",__FUNCTION__,__LINE__,*cls_index);
        ret = ca_classifier_rule_get(0,*cls_index,&get_priority,&get_key,&get_key_mask,&get_action);
        if(ret == CA_E_NOT_FOUND)
        {
            printk("%s %d : ca_classifier_rule_get index=%d Entry not found!!\n",__FUNCTION__,__LINE__,*cls_index);
            *cls_index = -1;
            return RT_ERR_OK;
        }
        OAM_CA_DRV_LOG(1,"%s %d : ca_classifier_rule_get cls_index=%d!!\n",__FUNCTION__,__LINE__,*cls_index);

//        printk("priority = %d\n",priority);
//        printk("key.src_port = 0x%x\n",key.src_port);
//        printk("key_mask.src_port = %d\n",key_mask.src_port);
//        printk("key.l2.vlan_count = %d\n",key.l2.vlan_count);
//        printk("key_mask.l2 = %d\n",key_mask.l2);
//        printk("key.l2.vlan_otag.vlan_max.vid = %d\n",key.l2.vlan_otag.vlan_max.vid);
//        printk("key.l2.vlan_otag.vlan_min.vid = %d\n",key.l2.vlan_otag.vlan_min.vid);
//        printk("key_mask.l2_mask.vlan_otag = %d\n",key_mask.l2_mask.vlan_otag);
//        printk("key_mask.l2_mask.vlan_otag_mask.vid = %d\n",key_mask.l2_mask.vlan_otag_mask.vid);
//        printk("action.forward = %d\n",action.forward);
//        printk("action.dest.port = 0x%x\n",action.dest.port);
//        printk("action.options.action_handle.llid_cos_index = 0x%x\n",action.options.action_handle.llid_cos_index);
//        printk("action.options.masks.action_handle = %d\n",action.options.masks.action_handle);
//        printk("action.options.outer_vlan_act = %d\n",action.options.outer_vlan_act);
//        printk("action.options.masks.outer_vlan_act = %d\n",action.options.masks.outer_vlan_act);
//
//        printk("get_priority = %d\n",get_priority);
//        printk("get_key.src_port = 0x%x\n",get_key.src_port);
//        printk("get_key_mask.src_port = %d\n",get_key_mask.src_port);
//        printk("get_key.l2.vlan_count = %d\n",get_key.l2.vlan_count);
//        printk("get_key_mask.l2 = %d\n",get_key_mask.l2);
//        printk("get_key.l2.vlan_otag.vlan_max.vid = %d\n",get_key.l2.vlan_otag.vlan_max.vid);
//        printk("get_key.l2.vlan_otag.vlan_min.vid = %d\n",get_key.l2.vlan_otag.vlan_min.vid);
//        printk("get_key_mask.l2_mask.vlan_otag = %d\n",get_key_mask.l2_mask.vlan_otag);
//        printk("get_key_mask.l2_mask.vlan_otag_mask.vid = %d\n",get_key_mask.l2_mask.vlan_otag_mask.vid);
//        printk("get_action.forward = %d\n",get_action.forward);
//        printk("get_action.dest.port = 0x%x\n",get_action.dest.port);
//        printk("get_action.options.action_handle.llid_cos_index = 0x%x\n",get_action.options.action_handle.llid_cos_index);
//        printk("get_action.options.masks.action_handle = %d\n",get_action.options.masks.action_handle);
//        printk("get_action.options.outer_vlan_act = %d\n",get_action.options.outer_vlan_act);
//        printk("get_action.options.masks.outer_vlan_act = %d\n",get_action.options.masks.outer_vlan_act);

        if(memcmp(&priority,&get_priority,sizeof(ca_uint32_t)))
        {
            printk("priority not match!!\n");
            *cls_index = -1;
            return RT_ERR_OK;
        }

        if(memcmp(&key,&get_key,sizeof(ca_classifier_key_t)))
        {
            printk("key not match!!\n");
            *cls_index = -1;
            return RT_ERR_OK;
        }

        if(memcmp(&key_mask,&get_key_mask,sizeof(ca_classifier_key_mask_t)))
        {
            printk("key_mask not match!!\n");
            *cls_index = -1;
            return RT_ERR_OK;
        }

        if(memcmp(&action,&get_action,sizeof(ca_classifier_action_t)))
        {
            printk("action not match!!\n");
            *cls_index = -1;
            return RT_ERR_OK;
        }

        OAM_CA_DRV_LOG(1,"%s %d : ca_classifier_rule_delete index=%d!!\n",__FUNCTION__,__LINE__,*cls_index);
        if((ret = ca_classifier_rule_delete(0,*cls_index)) != CA_E_OK)
        {
            printk("%s %d : ca_classifier_rule_delete index=%d Error ret=%d!!\n",__FUNCTION__,__LINE__,*cls_index,ret);
            return RT_ERR_FAILED;
        }
        OAM_CA_DRV_LOG(1,"%s %d : ca_classifier_rule_delete Success index=%d!!\n",__FUNCTION__,__LINE__,*cls_index);
        *cls_index = -1;
    }

    return RT_ERR_OK;
}

static int saturn_us_pon_rule_adding(void)
{
    int32_t ret = RT_ERR_OK;
    ca_uint32_t llid,cos,llid_cos_idx;

    for(llid=0;llid<MAX_LLID;llid++)
    {
        for(cos=0;cos<MAX_COS;cos++)
        {
            llid_cos_idx = llid*8+cos;
            if(llid_cos_idx < MAX_QUEUE)
            {
                if((ret = saturn_us_pon_rule_setting(llid,cos,&gbl_us_pon_cls_index[llid_cos_idx],1)) != RT_ERR_OK)
                {
                    printk("%s %d : saturn_us_pon_rule_setting llid=%d cos=%d Error ret=%d!!\n",__FUNCTION__,__LINE__,llid,cos,ret);
                    return RT_ERR_FAILED;
                }
            }
            else
            {
                printk("%s %d : PON Queue is only %d llid=%d cos=%d not support!!\n",__FUNCTION__,__LINE__,MAX_QUEUE,llid,cos);
                return RT_ERR_FAILED;
            }
        }
    }

    return RT_ERR_OK;
}

static int saturn_us_pon_rule_deleting(void)
{
    int32_t ret = RT_ERR_OK;
    ca_uint32_t llid,cos,llid_cos_idx;

    for(llid=0;llid<MAX_LLID;llid++)
    {
        for(cos=0;cos<MAX_COS;cos++)
        {
            llid_cos_idx = llid*8+cos;
            if(llid_cos_idx < MAX_QUEUE)
            {
                if((ret = saturn_us_pon_rule_setting(llid,cos,&gbl_us_pon_cls_index[llid_cos_idx],0)) != RT_ERR_OK)
                {
                    printk("%s %d : saturn_us_pon_rule_setting llid=%d cos=%d Error ret=%d!!\n",__FUNCTION__,__LINE__,llid,cos,ret);
                    return RT_ERR_FAILED;
                }
            }
            else
            {
                printk("%s %d : PON Queue is only %d llid=%d cos=%d not support!!\n",__FUNCTION__,__LINE__,MAX_QUEUE,llid,cos);
                return RT_ERR_FAILED;
            }
        }
    }
    return RT_ERR_OK;
}

static int oam_ca_drv_log_read(struct seq_file *seq, void *v)
{
    seq_printf(seq, "oam debug status: %u\n", oam_ca_drv_log);
    return 0;
}

static int oam_ca_drv_log_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    unsigned char tmpBuf[16] = {0};
    int len = (count > 15) ? 15 : count;
    
    if (buffer && !copy_from_user(tmpBuf, buffer, len))
    {
        oam_ca_drv_log = simple_strtoul(tmpBuf, NULL, 16);
        printk("\nwrite oam debug to %u\n", oam_ca_drv_log);
        return count;
    }
    return -EFAULT;
}

static int oam_ca_drv_log_open_proc(struct inode *inode, struct file *file)
{
    return single_open(file, oam_ca_drv_log_read, inode->i_private);
}

static const struct file_operations oam_ca_drv_log_fop = {
    .owner          = THIS_MODULE,
    .open           = oam_ca_drv_log_open_proc,
    .write          = oam_ca_drv_log_write,
    .read           = seq_read,
    .llseek         = seq_lseek,
    .release        = single_release,
};

static int mpcp_stats_read(struct seq_file *seq, void *v)
{
    int ret,i;
    ca_port_id_t port_id;
    ca_epon_mpcp_stats_t mpcp_stats;
    ca_epon_mpcp_llid_stats_t llid_stats;

    port_id = CA_PORT_ID(CA_PORT_TYPE_EPON,7);

    ret = ca_epon_mpcp_stats_get(0,port_id,0,&mpcp_stats);
    if(ret == CA_E_OK)
    {
        seq_printf(seq,"rx_mpcp_frames: %u\n",mpcp_stats.rx_mpcp_frames);         /* Indicates received MPCP packet number from 10G RxMAC*/
        seq_printf(seq,"rx_ext_mpcp_frames: %u\n",mpcp_stats.rx_ext_mpcp_frames);     /* Indicates received extended MPCP packet number from
                                                        10G RxMAC. Refer to table "Deregistration Cause" */
        seq_printf(seq,"rx_discovery_frames_drop: %u\n",mpcp_stats.rx_discovery_frames_drop);/* Indicates dropped discovery packet because of
                                                        discovery information mismatch between OLT and ONU */
        seq_printf(seq,"rx_ext_mpcp_frames_drop: %u\n",mpcp_stats.rx_ext_mpcp_frames_drop);/* Indicates dropped extended MPCP packet because of the overflow of buffer */
        for(i=0;i<CA_MPCGRANT_FRAME_DROP_MAX;i++)
            seq_printf(seq,"mpcp_stats.rx_grant_frames_drops[%d]: %u\n",i,mpcp_stats.rx_grant_frames_drops[i]);   /* Indicates the dropped MPCP grant number. System defined eight kinds of drop reasons */
        seq_printf(seq,"tx_mpcp_frames: %u\n",mpcp_stats.tx_mpcp_frames);         /* Indicates transmitted MPCP packet number to 10G TxMAC or 1G TxMAC */
        seq_printf(seq,"tx_ext_mpcp_frames: %u\n",mpcp_stats.tx_ext_mpcp_frames);     /* Indicates transmitted extended MPCP packet number to 10G TxMAC or 1G TxMAC */
    }

    for(i=0;i<AAL_EPON_LLID_NUM_MAX;i++)
    {
        ret = ca_epon_mpcp_llid_stats_get(0,port_id,i,0,&llid_stats);
        if(ret == CA_E_OK)
        {
            seq_printf(seq,"\t----llididx %d----: %u\n",i);
            seq_printf(seq,"\trx_good_octets: %u\n",llid_stats.rx_good_octets);     /* Received total good octets       */
            seq_printf(seq,"\trx_uc_frames: %u\n",llid_stats.rx_uc_frames);       /* Received unicast frames          */
            seq_printf(seq,"\trx_mc_frames: %u\n",llid_stats.rx_mc_frames);       /* Received multicast frames        */
            seq_printf(seq,"\trx_bc_frames: %u\n",llid_stats.rx_bc_frames);       /* Received broadcast frames        */
            seq_printf(seq,"\trx_oam_frames: %u\n",llid_stats.rx_oam_frames);      /* Received OAM frames          */
            seq_printf(seq,"\trx_crc8_valid_crc32_bad_frames: %u\n",llid_stats.rx_crc8_valid_crc32_bad_frames);/* The frames with bad CRC32 but correct CRC8 */
            seq_printf(seq,"\ttx_good_octets: %u\n",llid_stats.tx_good_octets);     /* Transmitted total good octets    */
            seq_printf(seq,"\ttx_uc_frames: %u\n",llid_stats.tx_uc_frames);       /* Transmitted unicast frames       */
            seq_printf(seq,"\ttx_mc_frames: %u\n",llid_stats.tx_mc_frames);       /* Transmitted multicast frames     */
            seq_printf(seq,"\ttx_bc_frames: %u\n",llid_stats.tx_bc_frames);       /* Transmitted broadcast frames     */
            seq_printf(seq,"\ttx_oam_frames: %u\n",llid_stats.tx_oam_frames);      /* Transmitted OAM frames           */
        }
    }

    return 0;
}

static int mpcp_stats_open_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    unsigned char tmpBuf[16] = {0};
    int len = (count > 15) ? 15 : count;
    int32_t ret = RT_ERR_OK;
    int i;
    ca_port_id_t port_id;
    ca_epon_mpcp_stats_t mpcp_stats;
    ca_epon_mpcp_llid_stats_t llid_stats;
    
    if (buffer && !copy_from_user(tmpBuf, buffer, len))
    {
        if(simple_strtoul(tmpBuf, NULL, 16)==1)
        {
            port_id = CA_PORT_ID(CA_PORT_TYPE_EPON,7);

            ca_epon_mpcp_stats_get(0,port_id,1,&mpcp_stats);
            for(i=0;i<AAL_EPON_LLID_NUM_MAX;i++)
            {
                ca_epon_mpcp_llid_stats_get(0,port_id,i,1,&llid_stats);
            }
        }

        return count;
    }
    return -EFAULT;
}

static int mpcp_stats_open_proc(struct inode *inode, struct file *file)
{
    return single_open(file, mpcp_stats_read, inode->i_private);
}

static const struct file_operations mpcp_stats_fop = {
    .owner          = THIS_MODULE,
    .open           = mpcp_stats_open_proc,
    .write          = mpcp_stats_open_write,
    .read           = seq_read,
    .llseek         = seq_lseek,
    .release        = single_release,
};

static int port_stats_read(struct seq_file *seq, void *v)
{
    int ret,i;
    ca_port_id_t port_id;
    ca_epon_port_stats_t port_stats;
    ca_epon_port_fec_stats_t fec_stats;

    port_id = CA_PORT_ID(CA_PORT_TYPE_EPON,7);

    ret = ca_epon_port_stats_get(0,port_id,0,&port_stats);
    if(ret == CA_E_OK)
    {
        seq_printf(seq,"rx_crc8_sld_err: %u\n",port_stats.rx_crc8_sld_err);        /* Number of received frames of which SLD or CRC8 is bad*/
        seq_printf(seq,"rx_bc_crc8_valid_fcs_err_frames: %u\n",port_stats.rx_bc_crc8_valid_fcs_err_frames);    /* Number of received broadcast frames of which CRC8 is good but FCS is bad */
        seq_printf(seq,"rx_ext_frames: %u\n",port_stats.rx_ext_frames);          /* Number of received MPCP extension frames.*/
        seq_printf(seq,"rx_good_octets: %u\n",port_stats.rx_good_octets);         /* Number of received octets without error.*/
        seq_printf(seq,"rx_uc_frames: %u\n",port_stats.rx_uc_frames);           /* Number of received unicast frames.*/
        seq_printf(seq,"rx_mc_frames: %u\n",port_stats.rx_mc_frames);           /* Number of received multicast frames. */
        seq_printf(seq,"rx_bc_frames: %u\n",port_stats.rx_bc_frames);           /* Number of received broadcast frames. */
        seq_printf(seq,"rx_fcs_error_frames: %u\n",port_stats.rx_fcs_error_frames);
        seq_printf(seq,"rx_oam_frames: %u\n",port_stats.rx_oam_frames);
        seq_printf(seq,"rx_pause_frames: %u\n",port_stats.rx_pause_frames);
        seq_printf(seq,"rx_unknown_oframes: %u\n",port_stats.rx_unknown_oframes);
        seq_printf(seq,"rx_runt_frames: %u\n",port_stats.rx_runt_frames);
        seq_printf(seq,"rx_gaint_frames: %u\n",port_stats.rx_gaint_frames);
        seq_printf(seq,"rx_64_frames: %u\n",port_stats.rx_64_frames);
        seq_printf(seq,"rx_65_127_frames: %u\n",port_stats.rx_65_127_frames);
        seq_printf(seq,"rx_128_255_frames: %u\n",port_stats.rx_128_255_frames);
        seq_printf(seq,"rx_256_511_frames: %u\n",port_stats.rx_256_511_frames);
        seq_printf(seq,"rx_512_1023_frames: %u\n",port_stats.rx_512_1023_frames);
        seq_printf(seq,"rx_1024_1518_frames: %u\n",port_stats.rx_1024_1518_frames);
        seq_printf(seq,"rx_1519_maxframes: %u\n",port_stats.rx_1519_maxframes);      /* frame size is equal to greater than 1519 bytes */
        seq_printf(seq,"tx_ext_frames: %u\n",port_stats.tx_ext_frames);
        seq_printf(seq,"tx_good_octets: %u\n",port_stats.tx_good_octets);
        seq_printf(seq,"tx_uc_frames: %u\n",port_stats.tx_uc_frames);
        seq_printf(seq,"tx_mc_frames: %u\n",port_stats.tx_mc_frames);
        seq_printf(seq,"tx_bc_frames: %u\n",port_stats.tx_bc_frames);
        seq_printf(seq,"tx_oam_frames: %u\n",port_stats.tx_oam_frames);
        seq_printf(seq,"tx_pause_frames: %u\n",port_stats.tx_pause_frames);
        seq_printf(seq,"tx_64_frames: %u\n",port_stats.tx_64_frames);
        seq_printf(seq,"tx_65_127_frames: %u\n",port_stats.tx_65_127_frames);
        seq_printf(seq,"tx_128_255_frames: %u\n",port_stats.tx_128_255_frames);
        seq_printf(seq,"tx_256_511_frames: %u\n",port_stats.tx_256_511_frames);
        seq_printf(seq,"tx_512_1023_frames: %u\n",port_stats.tx_512_1023_frames);
        seq_printf(seq,"tx_1024_1518_frames: %u\n",port_stats.tx_1024_1518_frames);
        seq_printf(seq,"tx_1519_maxframes: %u\n",port_stats.tx_1519_maxframes);      /* frame size is equal to or greater than 1519 bytes */
        seq_printf(seq,"mpcp_rx_bc_reg: %u\n",port_stats.mpcp_rx_bc_reg);         /* Number of received broadcast MPCP registration frames */
        seq_printf(seq,"mpcp_rx_bc_gate: %u\n",port_stats.mpcp_rx_bc_gate);        /* Number of received broadcast MPCP discovery gate frames. */
        seq_printf(seq,"mpcp_tx_mac_ctrl_frames: %u\n",port_stats.mpcp_tx_mac_ctrl_frames);    
        seq_printf(seq,"mpcp_tx_bc_reg_req: %u\n",port_stats.mpcp_tx_bc_reg_req);
        seq_printf(seq,"mpcp_tx_reg_ack: %u\n",port_stats.mpcp_tx_reg_ack);
        seq_printf(seq,"mpcp_rx_mac_ctrl_frames: %u\n",port_stats.mpcp_rx_mac_ctrl_frames);
    }

    ret = ca_epon_port_fec_stats_get(0,port_id,0,&fec_stats);
    if(ret == CA_E_OK)
    {
        seq_printf(seq,"corrected_bytes: %u\n",fec_stats.corrected_bytes);
        seq_printf(seq,"corrected_codewords: %u\n",fec_stats.corrected_codewords);
        seq_printf(seq,"uncorrectable_codewords: %u\n",fec_stats.uncorrectable_codewords);
        seq_printf(seq,"total_codewords: %u\n",fec_stats.total_codewords);
    }

    return 0;
}

static int port_stats_open_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    unsigned char tmpBuf[16] = {0};
    int len = (count > 15) ? 15 : count;
    int32_t ret = RT_ERR_OK;
    ca_port_id_t port_id;
    ca_epon_port_stats_t port_stats;
    ca_epon_port_fec_stats_t fec_stats;
    
    if (buffer && !copy_from_user(tmpBuf, buffer, len))
    {
        if(simple_strtoul(tmpBuf, NULL, 16)==1)
        {
            port_id = CA_PORT_ID(CA_PORT_TYPE_EPON,7);

            ca_epon_port_stats_get(0,port_id,1,&port_stats);
            ca_epon_port_fec_stats_get(0,port_id,1,&fec_stats);
        }

        return count;
    }
    return -EFAULT;
}

static int port_stats_open_proc(struct inode *inode, struct file *file)
{
    return single_open(file, port_stats_read, inode->i_private);
}

static const struct file_operations port_stats_fop = {
    .owner          = THIS_MODULE,
    .open           = port_stats_open_proc,
    .write          = port_stats_open_write,
    .read           = seq_read,
    .llseek         = seq_lseek,
    .release        = single_release,
};

static int us_pon_rule_read(struct seq_file *seq, void *v)
{
    int i;

    for(i=0;i<MAX_QUEUE;i++)
    {
        if(gbl_us_pon_cls_index[i]!=-1)
        {
            seq_printf(seq,"Saturn gbl_us_pon_cls_index[%d] = %d\n",i,gbl_us_pon_cls_index[i]);
        }
    }
    return 0;
}

static int us_pon_rule_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    unsigned char tmpBuf[16] = {0};
    int len = (count > 15) ? 15 : count;
    int32_t ret = RT_ERR_OK;
    
    if (buffer && !copy_from_user(tmpBuf, buffer, len))
    {
        if(simple_strtoul(tmpBuf, NULL, 16)==1)
        {
            saturn_us_pon_rule_adding();
        }
        else
        {
            saturn_us_pon_rule_deleting();
        }

        return count;
    }
    return -EFAULT;
}

static int us_pon_rule_open_proc(struct inode *inode, struct file *file)
{
    return single_open(file, us_pon_rule_read, inode->i_private);
}

static const struct file_operations us_pon_rule_fop = {
    .owner          = THIS_MODULE,
    .open           = us_pon_rule_open_proc,
    .write          = us_pon_rule_write,
    .read           = seq_read,
    .llseek         = seq_lseek,
    .release        = single_release,
};


static int oam_static_l2_addr_read(struct seq_file *seq, void *v)
{
    seq_printf(seq,"Saturn oam_static_l2_addr_setting = %d\n",oam_static_l2_addr_setting);
    return 0;
}

static int oam_static_l2_addr_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    unsigned char tmpBuf[16] = {0};
    int len = (count > 15) ? 15 : count;
    int32_t ret = RT_ERR_OK;
    
    if (buffer && !copy_from_user(tmpBuf, buffer, len))
    {
        if(simple_strtoul(tmpBuf, NULL, 16)==1)
        {
            oam_static_l2_addr_add();
        }
        else
        {
            oam_static_l2_addr_del();
        }

        return count;
    }
    return -EFAULT;
}

static int oam_static_l2_addr_open_proc(struct inode *inode, struct file *file)
{
    return single_open(file, oam_static_l2_addr_read, inode->i_private);
}

static const struct file_operations oam_static_l2_addr_fop = {
    .owner          = THIS_MODULE,
    .open           = oam_static_l2_addr_open_proc,
    .write          = oam_static_l2_addr_write,
    .read           = seq_read,
    .llseek         = seq_lseek,
    .release        = single_release,
};

static int mpcp_reg_start_read(struct seq_file *seq, void *v)
{
    int32_t ret = RT_ERR_OK;
    ca_boolean_t status = FALSE;

    ret = ca_epon_mpcp_registration_get(0, 0x20007, 0, &status);

    seq_printf(seq,"status=%d ret=%d\n",status,ret);
    return 0;
}

static int mpcp_reg_start_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    unsigned char tmpBuf[16] = {0};
    int len = (count > 15) ? 15 : count;
    int32_t ret = RT_ERR_OK;
    
    if (buffer && !copy_from_user(tmpBuf, buffer, len))
    {
        if(simple_strtoul(tmpBuf, NULL, 16)==1)
        {
            printk("Trigger MPCP Registration !!\n");
            if((ret = ca_epon_mpcp_registration_set(0, 0x20007, 0, 1))!=CA_E_OK)
            {
                printk("Trigger MPCP Registration ret=%d!!\n",ret);
            }
        }
        else
        {
            printk("Trigger MPCP De-registration !!\n");
            if((ret = ca_epon_mpcp_registration_set(0, 0x20007, 0, 0))!=CA_E_OK)
            {
                printk("Trigger MPCP De-registration ret=%d!!\n",ret);
            }
        }

        return count;
    }
    return -EFAULT;
}

static int mpcp_reg_start_open_proc(struct inode *inode, struct file *file)
{
    return single_open(file, mpcp_reg_start_read, inode->i_private);
}

static const struct file_operations mpcp_reg_start_fop = {
    .owner          = THIS_MODULE,
    .open           = mpcp_reg_start_open_proc,
    .write          = mpcp_reg_start_write,
    .read           = seq_read,
    .llseek         = seq_lseek,
    .release        = single_release,
};

static int pon_mac_reset_read(struct seq_file *seq, void *v)
{
    int32_t ret = RT_ERR_OK;
    ca_boolean_t status = FALSE;


    seq_printf(seq,"using echo 1 to reset\n",status,ret);
    return 0;
}

static int pon_mac_reset_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    unsigned char tmpBuf[16] = {0};
    int len = (count > 15) ? 15 : count;
    int32_t ret = RT_ERR_OK;
    ca_uint32_t pon_mode;
    
    if (buffer && !copy_from_user(tmpBuf, buffer, len))
    {
        if(simple_strtoul(tmpBuf, NULL, 16)==1)
        {
            pon_mode = aal_pon_mac_mode_get(0);

            if((ret = aal_psds_init(0, pon_mode))!=CA_E_OK)
            {
                printk("aal_psds_init ret=%d!!\n",ret);
            }
            
            if((ret = aal_epon_init(0, 0,pon_mode))!=CA_E_OK)
            {
                printk("aal_epon_init ret=%d!!\n",ret);
            }
        }

        return count;
    }
    return -EFAULT;
}

static int pon_mac_reset_open_proc(struct inode *inode, struct file *file)
{
    return single_open(file, pon_mac_reset_read, inode->i_private);
}

static const struct file_operations pon_mac_reset_fop = {
    .owner          = THIS_MODULE,
    .open           = pon_mac_reset_open_proc,
    .write          = pon_mac_reset_write,
    .read           = seq_read,
    .llseek         = seq_lseek,
    .release        = single_release,
};


int __init ca_rtk_epon_drv_init(void)
{
    int32_t ret = RT_ERR_OK;
    rtk_port_t pon;
    int ponPortMask,i;
    unsigned char voqMapMode;

    ret = scfg_read(0, CFG_ID_PON_VOQ_MODE, sizeof(voqMapMode), &voqMapMode);
    if (CA_E_OK != ret) {
        printk("<%s, %d> read CFG_ID_PON_VOQ_MODE fail\r\n", __func__, __LINE__);
    } else if (AAL_PUC_VOQ_MODE_8Q != voqMapMode){
        printk("<%s, %d> Fail! CFG_ID_PON_VOQ_MODE at /config/scfg.txt should be 0 \r\n", __func__, __LINE__);
    }

    ret = scfg_read(0, CFG_ID_PON_VOQ_MODE, sizeof(voqMapMode), &voqMapMode);

    for(i=0;i<MAX_QUEUE;i++)
    {
        gbl_us_pon_cls_index[i] = -1;
    }

    if (NULL == ca_rtk_epon_proc_dir) {
        ca_rtk_epon_proc_dir = proc_mkdir ("ca_rtk_epon", NULL);
    }

    if(ca_rtk_epon_proc_dir)
    {
        oam_ca_drv_log_entry = proc_create("oam_ca_drv_log", 0644, ca_rtk_epon_proc_dir, &oam_ca_drv_log_fop);
        if (!oam_ca_drv_log_entry) {
            printk("oam_ca_drv_log_entry, create proc failed!");
        }

        mpcp_stats_entry = proc_create("mpcp_stats", 0644, ca_rtk_epon_proc_dir, &mpcp_stats_fop);
        if (!mpcp_stats_entry) {
            printk("mpcp_stats_entry, create proc failed!");
        }

        port_stats_entry = proc_create("port_stats", 0644, ca_rtk_epon_proc_dir, &port_stats_fop);
        if (!port_stats_entry) {
            printk("port_stats_entry, create proc failed!");
        }

        us_pon_rule_entry = proc_create("us_pon_rule", 0644, ca_rtk_epon_proc_dir, &us_pon_rule_fop);
        if (!us_pon_rule_entry) {
            printk("us_pon_rule_entry, create proc failed!");
        }

        oam_static_l2_addr_entry = proc_create("oam_static_l2_addr", 0644, ca_rtk_epon_proc_dir, &oam_static_l2_addr_fop);
        if (!oam_static_l2_addr_entry) {
            printk("oam_static_l2_addr_entry, create proc failed!");
        }
        
        mpcp_reg_start_entry = proc_create("mpcp_reg_start", 0644, ca_rtk_epon_proc_dir, &mpcp_reg_start_fop);
        if (!mpcp_reg_start_entry) {
            printk("mpcp_reg_start_entry, create proc failed!");
        }

        pon_mac_reset_entry = proc_create("pon_mac_reset", 0644, ca_rtk_epon_proc_dir, &pon_mac_reset_fop);
        if (!pon_mac_reset_entry) {
            printk("pon_mac_reset_entry, create proc failed!");
        }
    }

    pon = HAL_GET_PON_PORT();
    ponPortMask = 1 << pon;

    if((ret = pkt_redirect_kernelApp_reg(PR_KERNEL_UID_GMAC, ca_rtk_epon_oam_pkt_tx))!=RT_ERR_OK)
    {
        OAM_CA_DRV_LOG(0,"OAM PR_KERNEL_UID_GMAC pkt_redirect_kernelApp_reg Error!!\n");
        return ret;
    }

    if((ret = pkt_redirect_kernelApp_reg(PR_KERNEL_UID_EPONDYINGGASP, ca_rtk_epon_oam_dyingGasp_tx))!=RT_ERR_OK)
    {
        OAM_CA_DRV_LOG(0,"OAM PR_KERNEL_UID_EPONDYINGGASP pkt_redirect_kernelApp_reg Error!!\n");
        return ret;
    }

    if((ret = drv_nic_register_rxhook(ponPortMask,1,&ca_rtk_epon_oam_pkt_rx))!=RT_ERR_OK)
    {
        OAM_CA_DRV_LOG(0,"OAM drv_nic_register_rxhook Error!!\n");
        return ret;
    }

    if((ret = ca_event_register(0,CA_EVENT_EPON_REG_STAT_CHNG,ca_rtk_epon_reg_intr,NULL))!=CA_E_OK)
    {
        OAM_CA_DRV_LOG(0,"OAM event epon stat changing event register Error!!\n");
        return ret;
    }

    if((ret = ca_event_register(0,CA_EVENT_EPON_PORT_LINK,ca_rtk_epon_port_intr,NULL))!=CA_E_OK)
    {
        OAM_CA_DRV_LOG(0,"OAM event epon port link register Error!!\n");
        return ret;
    }

    if((ret = ca_event_register(0,CA_EVENT_EPON_SILENCE_STATUS,ca_rtk_epon_silence_intr,NULL))!=CA_E_OK)
    {
        OAM_CA_DRV_LOG(0,"OAM event epon silence status register Error!!\n");
        return ret;
    }

    return 0;
}

void __exit ca_rtk_epon_drv_exit(void)
{
    int32_t ret = RT_ERR_OK;
    rtk_port_t pon;
    int ponPortMask;

    pon = HAL_GET_PON_PORT();
    ponPortMask = 1 << pon;

    if((ret = ca_event_deregister(0,CA_EVENT_EPON_REG_STAT_CHNG,ca_rtk_epon_reg_intr))!=CA_E_OK)
    {
        OAM_CA_DRV_LOG(0,"OAM event epon stat changing event deregister Error!!\n");
        return ret;
    }

    if((ret = ca_event_deregister(0,CA_EVENT_EPON_PORT_LINK,ca_rtk_epon_port_intr))!=CA_E_OK)
    {
        OAM_CA_DRV_LOG(0,"OAM event epon port link deregister Error!!\n");
        return ret;
    }

    if((ret = ca_event_deregister(0,CA_EVENT_EPON_SILENCE_STATUS,ca_rtk_epon_silence_intr))!=CA_E_OK)
    {
        OAM_CA_DRV_LOG(0,"OAM event epon silence status deregister Error!!\n");
        return ret;
    }

    if((ret = drv_nic_unregister_rxhook(ponPortMask,1,&ca_rtk_epon_oam_pkt_rx))!=RT_ERR_OK)
    {
        OAM_CA_DRV_LOG(0,"OAM drv_nic_unregister_rxhook Error!!\n");
    }

    if((ret = pkt_redirect_kernelApp_dereg(PR_KERNEL_UID_GMAC))!=RT_ERR_OK)
    {
        OAM_CA_DRV_LOG(0,"OAM PR_KERNEL_UID_GMAC pkt_redirect_kernelApp_dereg Error!!\n");
    }

    if((ret = pkt_redirect_kernelApp_dereg(PR_KERNEL_UID_EPONDYINGGASP))!=RT_ERR_OK)
    {
        OAM_CA_DRV_LOG(0,"OAM PR_KERNEL_UID_EPONDYINGGASP pkt_redirect_kernelApp_dereg Error!!\n");
    }

    if(oam_ca_drv_log_entry)
    {
        remove_proc_entry("oam_ca_drv_log", ca_rtk_epon_proc_dir);
        oam_ca_drv_log_entry = NULL;
    }

    if(mpcp_stats_entry)
    {
        remove_proc_entry("mpcp_stats", ca_rtk_epon_proc_dir);
        mpcp_stats_entry = NULL;
    }

    if(port_stats_entry)
    {
        remove_proc_entry("port_stats", ca_rtk_epon_proc_dir);
        port_stats_entry = NULL;
    }

    if(us_pon_rule_entry)
    {
        remove_proc_entry("us_pon_rule_rule", ca_rtk_epon_proc_dir);
        us_pon_rule_entry = NULL;
    }

    if(oam_static_l2_addr_entry)
    {
        remove_proc_entry("oam_static_l2_addr", ca_rtk_epon_proc_dir);
        oam_static_l2_addr_entry = NULL;
    }

    if(mpcp_reg_start_entry)
    {
        remove_proc_entry("mpcp_reg_start", ca_rtk_epon_proc_dir);
        mpcp_reg_start_entry = NULL;
    }
    
    if(pon_mac_reset_entry)
    {
        remove_proc_entry("pon_mac_reset", ca_rtk_epon_proc_dir);
        pon_mac_reset_entry = NULL;
    }

    if(ca_rtk_epon_proc_dir)
    {
        proc_remove(ca_rtk_epon_proc_dir);
        ca_rtk_epon_proc_dir = NULL;
    }
}


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CA RealTek EPON driver module");
MODULE_AUTHOR("Realtek");
module_init(ca_rtk_epon_drv_init);
module_exit(ca_rtk_epon_drv_exit);
EXPORT_SYMBOL(oam_ca_drv_log);
module_param(oam_ca_drv_log,int,0644);
EXPORT_SYMBOL(s_oam_register);
EXPORT_SYMBOL(s_oam_unregister);
#endif
