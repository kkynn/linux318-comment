/*
 *  a4 sta functions
 *
 *  $Id: 8192cd_a4_sta.c,v 1.1 2010/10/13 06:38:58 davidhsu Exp $
 *
 *  Copyright (c) 2010 Realtek Semiconductor Corp.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#define _8192CD_STA_CONTROL_C_

#ifdef __KERNEL__
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/timer.h>
#endif

#include "./8192cd_cfg.h"
#include "./8192cd.h"
#include "./8192cd_headers.h"
#include "./8192cd_debug.h"

#ifdef STA_CONTROL
//#define STA_CONTROL_DEBUGMSG
#ifdef STA_CONTROL_DEBUGMSG
unsigned char stactrl_debug_mac[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
#define STACTRL_DEBUG(mac, fmt, args...)     \
	do { \
		if(!memcmp(mac, stactrl_debug_mac, MACADDRLEN)) \
			panic_printk("[%s, %d][%s] "fmt" from %02x%02x%02x%02x%02x%02x\n",__func__,__LINE__, priv->dev->name, ## args, mac[0], mac[1],mac[2],mac[3],mac[4],mac[5]); \
	}while(0)

#define STACTRL_INIT_DEBUG(fmt, args...)  panic_printk("[%s, %d][%s] "fmt, __func__,__LINE__, priv->dev->name, ## args);
      
#else
#define STACTRL_DEBUG(mac, fmt, args...) {}
#define STACTRL_INIT_DEBUG
#endif

__inline__ static int stactrl_mac_hash(unsigned char *networkAddr, int hash_size)
{
    unsigned long x;

    x = networkAddr[0] ^ networkAddr[1] ^ networkAddr[2] ^ networkAddr[3] ^ networkAddr[4] ^ networkAddr[5];

    return x & (hash_size - 1);
}

__inline__ static void stactrl_mac_hash_link(struct stactrl_link_list *link, struct stactrl_link_list **head)
{
    link->next_hash = *head;

    if (link->next_hash != NULL)
        link->next_hash->pprev_hash = &link->next_hash;
    *head = link;
    link->pprev_hash = head;
}

__inline__ static void stactrl_mac_hash_unlink(struct stactrl_link_list *link)
{
    *(link->pprev_hash) = link->next_hash;
    if (link->next_hash != NULL)
        link->next_hash->pprev_hash = link->pprev_hash;
    link->next_hash = NULL;
    link->pprev_hash = NULL;
}

#ifdef STA_CONTROL_AIPC
#define NOT_EXIST -1

void stactrl_status_check(struct rtl8192cd_priv *priv)
{
    unsigned char prefer, status;
    if (priv->pmib->staControl.stactrl_enable == 0) return;
    
    aipc_wlan_get(STACTRL_WLAN_ID, &prefer, STACTRL_SHM_PREFER, 1, 0);
    aipc_wlan_get(STACTRL_WLAN_ID, &status, STACTRL_SHM_STATUS, 1, 0);

#if STA_CONTROL_ALGO == STA_CONTROL_ALGO2
    if ((0 == priv->stactrl.stactrl_status) && (1 == status)) {
        if (0 == prefer) {
            init_timer(&priv->stactrl.stactrl_timer);
            priv->stactrl.stactrl_timer.data = (unsigned long) priv;
            priv->stactrl.stactrl_timer.function = stactrl_non_prefer_expire;
            mod_timer(&priv->stactrl.stactrl_timer, jiffies + RTL_MILISECONDS_TO_JIFFIES(STACTRL_NON_PREFER_TIMER));
        }
    }
#endif
    priv->stactrl.stactrl_prefer = prefer;
    priv->stactrl.stactrl_status = status;
}
#endif


#ifdef CONFIG_RTL_PROC_NEW
int stactrl_info_read(struct seq_file *s, void *data)
#else
int stactrl_info_read(char *buf, char **start, off_t offset, int length, int *eof, void *data)
#endif
{
    struct net_device *dev = PROC_GET_DEV();
    struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
#ifdef STA_CONTROL_AIPC
    char           dev_name_sc[16];
    struct stactrl_prefer_shm prefer;
    struct stactrl_block_shm block;
#else
    struct stactrl_preferband_entry * prefer_ent;
    struct stactrl_block_entry * block_ent;
#endif

    int i;
    int pos = 0;
    PRINT_ONE(" -- sta control info -- ", "%s", 1);
    PRINT_ONE(priv->pmib->staControl.stactrl_enable,  "    MIB: sta control enable:  %d", 1);
    PRINT_ONE(priv->pmib->staControl.stactrl_groupID, "    MIB: sta control groupID: %d", 1);
    PRINT_ONE(priv->stactrl.stactrl_status, "    sta control status: %d", 1);
    PRINT_ONE(priv->stactrl.stactrl_prefer, "    is prefer band: %d", 1);
    PRINT_ONE(STA_CONTROL_ALGO, "    algorithm: %d", 1);

    if(priv->stactrl.stactrl_status)
    {
        if(priv->stactrl.stactrl_prefer)
        {
#ifdef STA_CONTROL_AIPC
            aipc_wlan_get(STACTRL_WLAN_ID_SC, dev_name_sc, STACTRL_SHM_DEV_NAME, sizeof(dev_name_sc), 0);
            aipc_wlan_get(STACTRL_WLAN_ID, (char *)&prefer.prefer_num, STACTRL_SHM_PREFER_NUM, 1, 0);
            aipc_wlan_get(STACTRL_WLAN_ID, (char *)&prefer.entry, STACTRL_SHM_PREFER_LIST, STACTRL_PREFERLIST_SIZE, 0);

            PRINT_ONE(dev_name_sc, "    non-prefer band: %s", 1);
            PRINT_ONE(prefer.prefer_num, "    -- prefer band capable client list: %d -- ", 1);
            if (prefer.prefer_num)
                PRINT_ONE("    Used    mac                  rssi    aging", "%s", 1);
            for (i = 0; i < prefer.prefer_num; i++)
            {
                int n = 0;
                unsigned char str[100];
                
                n = sprintf(str, "    %4d", prefer.entry[i].used);
                n += sprintf(str + n, "    "MACSTR, MAC2STR(prefer.entry[i].mac));
                n += sprintf(str + n, "    %4d", prefer.entry[i].rssi);
                n += sprintf(str + n, "    %5d", prefer.entry[i].aging);
                PRINT_ONE(str, "%s", 1);
            }

#else
            PRINT_ONE(priv->stactrl.stactrl_priv_sc->dev->name, "    non-prefer band: %s", 1);        
            PRINT_ONE("    -- prefer band capable client list -- ", "%s", 1);
            for (i=0; i<MAX_STACTRL_PREFERBAND_NUM; i++)
            {
                prefer_ent = &(priv->stactrl.stactrl_preferband_ent[i]);
                if (prefer_ent->used)
                {
                    PRINT_ARRAY_ARG("      STA_MAC: ",    prefer_ent->mac, "%02x", MACADDRLEN);
                    PRINT_ONE(prefer_ent->rssi,  "        rssi: %d", 1);
                    PRINT_ONE(prefer_ent->aging,  "        aging: %d", 1);
                }
            }
#endif // STA_CONTROL_AIPC
        }
        else
        {
#ifdef STA_CONTROL_AIPC
            aipc_wlan_get(STACTRL_WLAN_ID_SC, dev_name_sc, STACTRL_SHM_DEV_NAME, sizeof(dev_name_sc), 0);
            aipc_wlan_get(STACTRL_WLAN_ID, (char *)&block.block_num, STACTRL_SHM_BLOCK_NUM, 1, 0);
            aipc_wlan_get(STACTRL_WLAN_ID, (char *)&block.entry, STACTRL_SHM_BLOCK_LIST, STACTRL_BLOCKLIST_SIZE, 0);

            PRINT_ONE(dev_name_sc, "    prefer band: %s", 1);
            PRINT_ONE(block.block_num, "    -- non-prefer band capable client list: %d -- ", 1);
            if (block.block_num)
                PRINT_ONE("    Used    mac                  timerX    retryY", "%s", 1);
            for (i = 0; i < block.block_num; i++)
            {
                int n = 0;
                unsigned char str[100];

                n = sprintf(str, "    %4d", block.entry[i].used);
                n += sprintf(str + n, "    "MACSTR, MAC2STR(block.entry[i].mac));
                n += sprintf(str + n, "    %6d", block.entry[i].timerX);
                n += sprintf(str + n, "    %6d", block.entry[i].retryY);
                PRINT_ONE(str, "%s", 1);
            }

#else
            PRINT_ONE(priv->stactrl.stactrl_priv_sc->dev->name, "    prefer band: %s", 1);                
            PRINT_ONE("    -- non-prefer band blocklist -- ", "%s", 1);
            for (i=0; i<MAX_STACTRL_BLOCK_NUM; i++)
            {
                block_ent = &(priv->stactrl.stactrl_block_ent[i]);
                if (block_ent->used)
                {
                    PRINT_ARRAY_ARG("      STA_MAC: ",    block_ent->mac, "%02x", MACADDRLEN);
                    PRINT_ONE(block_ent->timerX,  "        timerX: %d", 1);
                    PRINT_ONE(block_ent->retryY,  "        retryY: %d", 1);
                }
            }
#endif
        }
    }

#ifdef STA_CONTROL_DEBUGMSG
    PRINT_ONE("    -- debug mac -- ", "%s", 1);
    PRINT_ARRAY_ARG("      STA_MAC: ",    stactrl_debug_mac, "%02x", MACADDRLEN);
#endif
    return pos;
}

int stactrl_info_write(struct file *file, const char *buffer,
                  unsigned long count, void *data)
{
#ifdef STA_CONTROL_DEBUGMSG
    if (buffer)
    {
        get_array_val(stactrl_debug_mac, (char *)buffer, count);
    }
#endif
    return count;
}


#ifdef STA_CONTROL_AIPC
void stactrl_init(struct rtl8192cd_priv *priv)
{
    unsigned char  stactrl_wrt[4];
    unsigned char  stactrl_read[25];
    unsigned int   opmode_sc;
    char           dev_name_sc[16];
    int            find_sc = 0;
    unsigned char  prefer_sc;
    char empty[STA_CONTROL_SHM_SIZE] = {0};
    struct stactrl_shm_common shm_cmn, shm_cmn_sc;

    STACTRL_INIT_DEBUG("groupID: %d\n",  priv->pmib->staControl.stactrl_groupID);
    if (priv->pmib->staControl.stactrl_enable > 1) priv->pmib->staControl.stactrl_enable = 1;
    if (priv->pmib->staControl.stactrl_prefer_band > 1) priv->pmib->staControl.stactrl_prefer_band = 1;
    priv->stactrl.stactrl_status = 0;

    if (!(OPMODE & WIFI_AP_STATE) || priv->pmib->staControl.stactrl_enable == 0) {
        STACTRL_INIT_DEBUG("WARNING: not ap mode or STA_CONTROL is not enabled\n");
        return;
    }

    /* initialize the list */
    aipc_wlan_set(STACTRL_WLAN_ID, empty, 0, STA_CONTROL_SHM_SIZE, 0);

    shm_cmn.enable = priv->pmib->staControl.stactrl_enable;
    shm_cmn.group_id = priv->pmib->staControl.stactrl_groupID;
    shm_cmn.prefer = priv->pmib->staControl.stactrl_prefer_band;
    shm_cmn.status = priv->stactrl.stactrl_status;
    shm_cmn.driver_state = 0;
    shm_cmn.opmode = priv->pmib->dot11OperationEntry.opmode;
    shm_cmn.assoc_num = priv->assoc_num;

    memcpy(shm_cmn.dev_name, priv->dev->name, IFNAMSIZ);
#ifdef CONFIG_ARCH_LUNA_SLAVE
    shm_cmn.dev_name[4] = '1';
#endif

    aipc_wlan_set(STACTRL_WLAN_ID, (char *)&shm_cmn.enable, STACTRL_SHM_ENABLE, sizeof(struct stactrl_shm_common), 0);
    aipc_wlan_get(STACTRL_WLAN_ID_SC, (char *)&shm_cmn_sc.enable, STACTRL_SHM_ENABLE, sizeof(struct stactrl_shm_common), 0);

    if (shm_cmn_sc.driver_state) {
        if (shm_cmn_sc.enable && 
            shm_cmn_sc.group_id == shm_cmn.group_id && 
            !shm_cmn_sc.status && 
            (shm_cmn_sc.opmode & WIFI_AP_STATE)) {

            find_sc = 1;
        }
    }

    if (find_sc) {
        memcpy(dev_name_sc, &shm_cmn_sc.dev_name, IFNAMSIZ);
        STACTRL_INIT_DEBUG("find other band: %s\n", dev_name_sc);

        /* set stactrl_prefer into shared memory */
        shm_cmn.prefer = priv->pmib->staControl.stactrl_prefer_band ? 0 : 1;
        aipc_wlan_set(STACTRL_WLAN_ID_SC, (char *)&shm_cmn.prefer, STACTRL_SHM_PREFER, 1, 0);

        /* set stactrl_status = 1 into shared memory */
        shm_cmn.status = 1;
        aipc_wlan_set(STACTRL_WLAN_ID, (char *)&shm_cmn.status, STACTRL_SHM_STATUS, 1, 0);
        aipc_wlan_set(STACTRL_WLAN_ID_SC, (char *)&shm_cmn.status, STACTRL_SHM_STATUS, 1, 0);

        priv->stactrl.stactrl_prefer = priv->pmib->staControl.stactrl_prefer_band;
        priv->stactrl.stactrl_status = 1;
		
#if STA_CONTROL_ALGO == STA_CONTROL_ALGO2
        if (0 == priv->stactrl.stactrl_prefer) {
            init_timer(&priv->stactrl.stactrl_timer);
            priv->stactrl.stactrl_timer.data = (unsigned long) priv;
            priv->stactrl.stactrl_timer.function = stactrl_non_prefer_expire;
            mod_timer(&priv->stactrl.stactrl_timer, jiffies + RTL_MILISECONDS_TO_JIFFIES(STACTRL_NON_PREFER_TIMER));
        }
#endif
    } else {
        /* didn't find available device */
        STACTRL_INIT_DEBUG("WARNING: not find available device\n");
    }
	
#ifdef SMP_SYNC
    spin_lock_init(&(priv->stactrl.stactrl_lock));
#endif
}
#else
void stactrl_init(struct rtl8192cd_priv *priv)
{
    struct rtl8192cd_priv *priv_sc;
    struct rtl8192cd_priv *prefer_priv;
    int i;
    
    STACTRL_INIT_DEBUG("groupID: %d\n",  priv->pmib->staControl.stactrl_groupID);

    if(priv->pmib->staControl.stactrl_enable > 1)
        priv->pmib->staControl.stactrl_enable = 1;
    if(priv->pmib->staControl.stactrl_prefer_band > 1)
        priv->pmib->staControl.stactrl_prefer_band = 1;
    
    priv->stactrl.stactrl_status = 0;
    priv->stactrl.stactrl_priv_sc = NULL;

    if(!(OPMODE & WIFI_AP_STATE) || priv->pmib->staControl.stactrl_enable == 0) {
        STACTRL_INIT_DEBUG("WARNING: not ap mode or not enabled\n");
        return;

    }
    
    /*search other band to find the priv with same groupID*/
    for(i = 0; i < RTL8192CD_NUM_VWLAN+1; i++) {
        if(i == 0) {
            priv_sc = GET_ROOT(priv)->stactrl.stactrl_rootpriv_sc;
        }
        #ifdef MBSSID
        else { /*search vap of other band*/
            priv_sc = GET_ROOT(priv)->stactrl.stactrl_rootpriv_sc->pvap_priv[i - 1];

        }
        #endif
        /*find other band's priv with same groupID*/
        if(priv_sc->pmib->staControl.stactrl_groupID == priv->pmib->staControl.stactrl_groupID &&
             IS_DRV_OPEN(priv_sc) && 
             priv_sc->pmib->staControl.stactrl_enable == 1 &&
             (priv_sc->pmib->dot11OperationEntry.opmode & WIFI_AP_STATE) &&
             priv_sc->stactrl.stactrl_status == 0) 
         {        
             priv->stactrl.stactrl_priv_sc = priv_sc;
             break;
         }        
    }

    
    if(priv->stactrl.stactrl_priv_sc == NULL) { /*can not find other band with same groupID*/        
        STACTRL_INIT_DEBUG("WARNING: not find available priv\n");
        return;
    }
    
    STACTRL_INIT_DEBUG("find other band : %s\n", priv->stactrl.stactrl_priv_sc->dev->name);
    priv_sc->stactrl.stactrl_priv_sc = priv;
    

    /*setting prefer band and non-prefer band*/
	priv->stactrl.stactrl_prefer = priv->pmib->staControl.stactrl_prefer_band;
    priv->stactrl.stactrl_priv_sc->stactrl.stactrl_prefer = priv->pmib->staControl.stactrl_prefer_band?0:1;


    /* memory allocated structure*/
    if(priv->stactrl.stactrl_prefer) {
        prefer_priv = priv;
    }
    else {
        prefer_priv = priv->stactrl.stactrl_priv_sc;
    }
    priv_sc = prefer_priv->stactrl.stactrl_priv_sc;


    STACTRL_INIT_DEBUG("prefer band : %s, non-prefer band: %s\n", prefer_priv->dev->name, priv_sc->dev->name);

    
    prefer_priv->stactrl.stactrl_preferband_ent = (struct stactrl_preferband_entry *)
                kmalloc((sizeof(struct stactrl_preferband_entry) * MAX_STACTRL_PREFERBAND_NUM), GFP_ATOMIC);
    if (!prefer_priv->stactrl.stactrl_preferband_ent) {
        printk(KERN_ERR "Can't kmalloc for stactrl_preferband_entry (size %d)\n", sizeof(struct stactrl_preferband_entry) * MAX_STACTRL_PREFERBAND_NUM);
        goto err;
    }
    memset(prefer_priv->stactrl.stactrl_preferband_machash, 0, sizeof(prefer_priv->stactrl.stactrl_preferband_machash));
    memset(prefer_priv->stactrl.stactrl_preferband_ent, 0, sizeof(struct stactrl_preferband_entry) * MAX_STACTRL_PREFERBAND_NUM);     
    

    priv_sc->stactrl.stactrl_block_ent = (struct stactrl_block_entry *)
                kmalloc((sizeof(struct stactrl_block_entry) * MAX_STACTRL_BLOCK_NUM), GFP_ATOMIC);
    if (!priv_sc->stactrl.stactrl_block_ent) {
        printk(KERN_ERR "Can't kmalloc for stactrl_preferband_entry (size %d)\n", sizeof(struct stactrl_block_entry) * MAX_STACTRL_BLOCK_NUM);
        goto err;
    }          
    memset(priv_sc->stactrl.stactrl_block_machash, 0, sizeof(priv_sc->stactrl.stactrl_block_machash));
    memset(priv_sc->stactrl.stactrl_block_ent, 0, sizeof(struct stactrl_block_entry) * MAX_STACTRL_BLOCK_NUM);

    
#if STA_CONTROL_ALGO == STA_CONTROL_ALGO2
    init_timer(&priv_sc->stactrl.stactrl_timer);
    priv_sc->stactrl.stactrl_timer.data = (unsigned long) priv_sc;
    priv_sc->stactrl.stactrl_timer.function = stactrl_non_prefer_expire;
    mod_timer(&priv_sc->stactrl.stactrl_timer, jiffies + RTL_MILISECONDS_TO_JIFFIES(STACTRL_NON_PREFER_TIMER));    
#endif

#ifdef SMP_SYNC
    spin_lock_init(&(prefer_priv->stactrl.stactrl_lock));
    spin_lock_init(&(priv_sc->stactrl.stactrl_lock));
#endif

    /*start up sta control*/
    prefer_priv->stactrl.stactrl_status = 1;
    priv_sc->stactrl.stactrl_status = 1;
    return;


err:
    if(prefer_priv->stactrl.stactrl_preferband_ent)
        kfree(prefer_priv->stactrl.stactrl_preferband_ent);
    if(priv_sc->stactrl.stactrl_block_ent) 
        kfree(priv_sc->stactrl.stactrl_block_ent);

    return;
}
#endif // STA_CONTROL_AIPC


#ifdef STA_CONTROL_AIPC
void stactrl_deinit(struct rtl8192cd_priv *priv)
{
    unsigned char stactrl_status;

    aipc_wlan_get(STACTRL_WLAN_ID, &stactrl_status, STACTRL_SHM_STATUS, sizeof(stactrl_status), 0);
    if(stactrl_status) {
        stactrl_status = 0;
        aipc_wlan_set(STACTRL_WLAN_ID, &stactrl_status, STACTRL_SHM_STATUS, sizeof(stactrl_status), 0);
        aipc_wlan_set(STACTRL_WLAN_ID_SC, &stactrl_status, STACTRL_SHM_STATUS, sizeof(stactrl_status), 0);
#if STA_CONTROL_ALGO == STA_CONTROL_ALGO2
        if(0 == priv->stactrl.stactrl_prefer)
            if (timer_pending(&priv->stactrl.stactrl_timer)) 
                del_timer(&priv->stactrl.stactrl_timer);
#endif
    }
    priv->stactrl.stactrl_status = 0;
}
#else
void stactrl_deinit(struct rtl8192cd_priv *priv)
{
    struct rtl8192cd_priv *priv_sc;
    struct rtl8192cd_priv *prefer_priv;

    STACTRL_INIT_DEBUG("\n");
    if(priv->stactrl.stactrl_status) {
        if(priv->stactrl.stactrl_prefer) {
            prefer_priv = priv;
        }
        else {
            prefer_priv = priv->stactrl.stactrl_priv_sc;
        }
        priv_sc = prefer_priv->stactrl.stactrl_priv_sc;

        STACTRL_INIT_DEBUG("prefer band : %s, non-prefer band: %s\n", prefer_priv->dev->name, priv_sc->dev->name);
        
        if(prefer_priv->stactrl.stactrl_preferband_ent)
            kfree(prefer_priv->stactrl.stactrl_preferband_ent);
        if(priv_sc->stactrl.stactrl_block_ent) 
            kfree(priv_sc->stactrl.stactrl_block_ent);        
        
        prefer_priv->stactrl.stactrl_status = 0;
        priv_sc->stactrl.stactrl_status = 0;
        
#if STA_CONTROL_ALGO == STA_CONTROL_ALGO2
        if (timer_pending(&priv_sc->stactrl.stactrl_timer))
        {
            del_timer(&priv_sc->stactrl.stactrl_timer);
        }
#endif
    }
}
#endif // STA_CONTROL_AIPC


#ifdef STA_CONTROL_AIPC
static int stactrl_block_lookup(struct rtl8192cd_priv *priv, unsigned char *mac, struct stactrl_block_shm *block)
{
    int i, id;

    id = (priv->stactrl.stactrl_prefer) ? (STACTRL_WLAN_ID_SC) : (STACTRL_WLAN_ID);

    aipc_wlan_get(id, (char *)block, STACTRL_SHM_BASE, STACTRL_BLOCK_SIZE, 0);

    for (i = 0; i < block->block_num; i++) {
        if (block->entry[i].used && 
            !memcmp(block->entry[i].mac, mac, MACADDRLEN)) {

            return i;
        }
    }
    return NOT_EXIST;
}
#else
static struct stactrl_block_entry *stactrl_block_lookup(struct rtl8192cd_priv *priv, unsigned char *mac)
{
    unsigned long offset;
    int hash;
    struct stactrl_link_list *link;
    struct stactrl_block_entry * ent;

    offset = (unsigned long)(&((struct stactrl_block_entry *)0)->link_list);
    hash = stactrl_mac_hash(mac, STACTRL_BLOCK_HASH_SIZE);
    link = priv->stactrl.stactrl_block_machash[hash];
    while (link != NULL)
    {
        ent = (struct stactrl_block_entry *)((unsigned long)link - offset);
        if (!memcmp(ent->mac, mac, MACADDRLEN))
        {
            return ent;
        }
        link = link->next_hash;
    }

    return NULL;

}
#endif // STA_CONTROL_AIPC


#ifdef STA_CONTROL_AIPC
static int stactrl_block_add(struct rtl8192cd_priv *priv, unsigned char *mac, struct stactrl_block_shm *block)
{
    int            index = NOT_EXIST;
    int            ret = 0;

    /* if there is a empty entry in the block-list */
    if (block->block_num < MAX_STACTRL_BLOCK_NUM) {
        index = block->block_num;
        block->block_num += 1;
        block->entry[index].used = 1;
        block->entry[index].timerX = STACTRL_BLOCK_EXPIRE_X;
        block->entry[index].retryY = STACTRL_BLOCK_RETRY_Y;
        block->entry[index].timerZ = 0;
        memcpy(block->entry[index].mac, mac, MACADDRLEN);

        aipc_wlan_set(STACTRL_WLAN_ID, (char *)&block->block_num, STACTRL_SHM_BLOCK_NUM, 1, 0);
        aipc_wlan_set(STACTRL_WLAN_ID, (char *)&block->entry[index], 
            STACTRL_SHM_BLOCK_LIST + index * STACTRL_BLOCK_ENTRY_SIZE, STACTRL_BLOCK_ENTRY_SIZE, 0);

        ret = 1;
    }
    return ret;
}
#else
/*return value: NULL: error,  other:success*/
static struct stactrl_block_entry * stactrl_block_add(struct rtl8192cd_priv *priv, unsigned char *mac)
{
    struct stactrl_block_entry * ent = NULL;
    int i, hash;
    ASSERT(mac);

	ent = stactrl_block_lookup(priv, mac); //Avoid exist mac to add again
 	if(ent != NULL)
		return ent;

    /* find an empty entry*/
    for (i=0; i<MAX_STACTRL_BLOCK_NUM; i++)
    {
        if (!priv->stactrl.stactrl_block_ent[i].used)
        {
            ent = &(priv->stactrl.stactrl_block_ent[i]);
            break;
        }
    }

    if(ent)
    {
        ent->used = 1;
        memcpy(ent->mac, mac, MACADDRLEN);
        ent->retryY = STACTRL_BLOCK_RETRY_Y;
        ent->timerX = STACTRL_BLOCK_EXPIRE_X;
        ent->timerZ = 0;
        hash = stactrl_mac_hash(mac, STACTRL_BLOCK_HASH_SIZE);
        stactrl_mac_hash_link(&(ent->link_list), &(priv->stactrl.stactrl_block_machash[hash]));
        return ent;
    }

    return NULL;
}
#endif // STA_CONTROL_AIPC


#ifdef STA_CONTROL_AIPC
/* prefer-band set a flag to let block-band know which should be deleted */
static void stactrl_block_delete(struct rtl8192cd_priv *priv, unsigned char *mac, struct stactrl_block_shm *block)
{
    int           i;

    aipc_wlan_get(STACTRL_WLAN_ID_SC, (char *)&block->block_num, STACTRL_SHM_BLOCK_NUM, 1, 0);
    aipc_wlan_get(STACTRL_WLAN_ID_SC, (char *)&block->entry, STACTRL_SHM_BLOCK_LIST, STACTRL_BLOCKLIST_SIZE, 0);

    for (i = 0; i < block->block_num; i++) {
        if (!memcmp(block->entry[i].mac, mac, MACADDRLEN)) {
            block->entry[i].used = 0;
            aipc_wlan_set(STACTRL_WLAN_ID_SC, (char *)&block->entry[i], 
                STACTRL_SHM_BLOCK_LIST + i * STACTRL_BLOCK_ENTRY_SIZE, STACTRL_BLOCK_ENTRY_SIZE, 0);
            return;
        }
    }
}
#else
/*return value: 0:success, other: error*/
static int stactrl_block_delete(struct rtl8192cd_priv *priv, unsigned char *mac)
{
    unsigned long offset;
    int hash;
    struct stactrl_link_list *link;
    struct stactrl_block_entry * ent;
    unsigned long flags;
    
    SAVE_INT_AND_CLI(flags);
    SMP_LOCK_STACONTROL_LIST(flags);

    offset = (unsigned long)(&((struct stactrl_block_entry *)0)->link_list);
    hash = stactrl_mac_hash(mac, STACTRL_BLOCK_HASH_SIZE);
    link = priv->stactrl.stactrl_block_machash[hash];
    while (link != NULL)
    {
        ent = (struct stactrl_block_entry *)((unsigned long)link - offset);
        if (!memcmp(ent->mac, mac, MACADDRLEN))
        {
            ent->used = 0;
            stactrl_mac_hash_unlink(link);
            STACTRL_DEBUG(ent->mac, "block delete");    
            SMP_UNLOCK_STACONTROL_LIST(flags);
            RESTORE_INT(flags);            
            return 0;
        }
        link = link->next_hash;
    }

    SMP_UNLOCK_STACONTROL_LIST(flags);
    RESTORE_INT(flags);
    return 1;

}
#endif


#ifdef STA_CONTROL_AIPC
static int stactrl_preferband_sta_lookup(struct rtl8192cd_priv *priv, unsigned char *mac, struct stactrl_prefer_shm *prefer)
{
    int i, id;

    id = (priv->stactrl.stactrl_prefer) ? (STACTRL_WLAN_ID) : (STACTRL_WLAN_ID_SC);

    aipc_wlan_get(id, (char *)prefer, STACTRL_SHM_BASE, STACTRL_PREFER_SIZE, 0); 

    for (i = 0; i < prefer->prefer_num; i++) {
        if (prefer->entry[i].used &&
            !memcmp(prefer->entry[i].mac, mac, MACADDRLEN)) {
            return i;
        }
    }
    return NOT_EXIST;
}
#else
static struct stactrl_preferband_entry *stactrl_preferband_sta_lookup(struct rtl8192cd_priv *priv, unsigned char *mac)
{
    unsigned long offset;
    int hash;
    struct stactrl_link_list *link;
    struct stactrl_preferband_entry * ent;

    offset = (unsigned long)(&((struct stactrl_preferband_entry *)0)->link_list);
    hash = stactrl_mac_hash(mac, STACTRL_PREFERBAND_HASH_SIZE);
    link = priv->stactrl.stactrl_preferband_machash[hash];
    while (link != NULL)
    {
        ent = (struct stactrl_preferband_entry *)((unsigned long)link - offset);
        if (!memcmp(ent->mac, mac, MACADDRLEN))
        {
            return ent;
        }
        link = link->next_hash;
    }

    return NULL;

}
#endif // STA_CONTROL_AIPC


#ifdef STA_CONTROL_AIPC
void stactrl_preferband_sta_add(struct rtl8192cd_priv *priv, unsigned char *mac, unsigned char rssi)
{
    int           i;
    int           index = NOT_EXIST;
    int           num_sta;

    int           block_delete_flag = 0;
    unsigned int  max_aging, aging;
    unsigned long flags = 0;
    struct stactrl_prefer_shm prefer;
    struct stactrl_block_shm block;

    SAVE_INT_AND_CLI(flags);
    SMP_LOCK_STACONTROL_LIST(flags);
	
    index = stactrl_preferband_sta_lookup(priv, mac, &prefer);
    if (prefer.status) {
        num_sta = prefer.prefer_num;
        /* if find the entry in the prefer-band list */
        if (NOT_EXIST != index) {
            if (rssi < STACTRL_PREFERBAND_RSSI - STACTRL_PREFERBAND_RSSI_TOLERANCE) {
                /* replace "index" one with the last one */
                memcpy((void *)&prefer.entry[index], (void *)&prefer.entry[num_sta - 1], STACTRL_PREFER_ENTRY_SIZE);
                memset((void *)&prefer.entry[num_sta - 1], 0, STACTRL_PREFER_ENTRY_SIZE);
                block_delete_flag = 1;
                aipc_wlan_set(STACTRL_WLAN_ID, (char *)&prefer.entry, 
                    STACTRL_SHM_PREFER_LIST, num_sta * STACTRL_PREFER_ENTRY_SIZE, 0);
            } else {
                /* update the station in the prefer-band list */            
                prefer.entry[index].rssi = rssi;
                prefer.entry[index].aging = 0;
                aipc_wlan_set(STACTRL_WLAN_ID, (char *)&prefer.entry[index], 
                    STACTRL_SHM_PREFER_LIST + index * STACTRL_PREFER_ENTRY_SIZE, STACTRL_PREFER_ENTRY_SIZE, 0);
            }
        } else {
            /* if didn't find any entry in the prefer-band list */        
            if (rssi >= STACTRL_PREFERBAND_RSSI) {
                /* find the first empty entry */
                if (num_sta < MAX_STACTRL_PREFERBAND_NUM) {
                    index = num_sta;
                    /* total number of station + 1 */
                    num_sta += 1;
                    prefer.prefer_num += 1;
                } else if (num_sta == MAX_STACTRL_PREFERBAND_NUM) {
                    /* find an entry with max aging */                
                    max_aging = 0;
                    for (i = 0; i < MAX_STACTRL_PREFERBAND_NUM; i++) {
                        aging = prefer.entry[i].aging;
                        if (max_aging < aging) {
                            max_aging = aging;
                            index = i;
                        }
                    }
                }

                /* insert the new station */
                prefer.entry[index].used = 1;
                prefer.entry[index].rssi = rssi;
                prefer.entry[index].aging = 0;
                memcpy(prefer.entry[index].mac, mac, MACADDRLEN);

                aipc_wlan_set(STACTRL_WLAN_ID, (char *)&prefer.prefer_num, STACTRL_SHM_PREFER_NUM, 1, 0);
                aipc_wlan_set(STACTRL_WLAN_ID, (char *)&prefer.entry, STACTRL_SHM_PREFER_LIST, 
                    num_sta * STACTRL_PREFER_ENTRY_SIZE, 0);
            }
        }
    } else {
        priv->stactrl.stactrl_status = 0;
    }
	
    SMP_UNLOCK_STACONTROL_LIST(flags);
    RESTORE_INT(flags);
	
    if (block_delete_flag)
        stactrl_block_delete(priv, mac, &block);

}
#else
void stactrl_preferband_sta_add(struct rtl8192cd_priv *priv, unsigned char *mac, unsigned char rssi)
{
    struct stactrl_preferband_entry * ent;
    int i, hash;
    int temp_aging, temp_index;
    unsigned long flags = 0;
    ASSERT(mac);

    STACTRL_DEBUG(mac, "receive probe");
    SAVE_INT_AND_CLI(flags);
    SMP_LOCK_STACONTROL_LIST(flags);
    ent = stactrl_preferband_sta_lookup(priv, mac);
    if(ent) /*find existed entry*/
    {
        if(rssi < STACTRL_PREFERBAND_RSSI - STACTRL_PREFERBAND_RSSI_TOLERANCE)
        {
            ent->used = 0;
            stactrl_mac_hash_unlink(&(ent->link_list));
            SMP_UNLOCK_STACONTROL_LIST(flags);
            RESTORE_INT(flags);
            
            stactrl_block_delete(priv->stactrl.stactrl_priv_sc, mac);
            return;
        }
        else
        {
            ent->aging = 0;
            ent->rssi = rssi;
            
        }
        goto ret;
    }

    if(rssi < STACTRL_PREFERBAND_RSSI)
    {
        goto ret;
    }

    /* find an empty entry*/
    ent = NULL;
    for (i=0; i<MAX_STACTRL_PREFERBAND_NUM; i++)
    {
        if (!priv->stactrl.stactrl_preferband_ent[i].used)
        {
            ent = &(priv->stactrl.stactrl_preferband_ent[i]);
            break;
        }
    }

    /* not found, find a entry with max aging*/
    if(ent == NULL)
    {
        temp_aging = 0;
        temp_index = 0;
        for (i=0; i<MAX_STACTRL_PREFERBAND_NUM; i++)
        {
            if(priv->stactrl.stactrl_preferband_ent[i].used && temp_aging < priv->stactrl.stactrl_preferband_ent[i].aging)
            {
                temp_aging = priv->stactrl.stactrl_preferband_ent[i].aging;
                temp_index = i;
            }

        }
        ent = &(priv->stactrl.stactrl_preferband_ent[temp_index]);
        stactrl_mac_hash_unlink(&(ent->link_list));
    }

    ent->used = 1;
    memcpy(ent->mac, mac, MACADDRLEN);
    ent->aging = 0;
    ent->rssi = rssi;
    hash = stactrl_mac_hash(mac, STACTRL_PREFERBAND_HASH_SIZE);
    stactrl_mac_hash_link(&(ent->link_list), &(priv->stactrl.stactrl_preferband_machash[hash]));

ret:
    SMP_UNLOCK_STACONTROL_LIST(flags);
    RESTORE_INT(flags);
}
#endif // STA_CONTROL_AIPC


#ifdef STA_CONTROL_AIPC
static unsigned char stactrl_is_need(struct rtl8192cd_priv *priv, unsigned char *mac)
{
    int           index = NOT_EXIST;
    unsigned char ret = 0;
    struct stactrl_prefer_shm prefer;

    index = stactrl_preferband_sta_lookup(priv, mac, &prefer);
    if ((NOT_EXIST != index) && prefer.assoc_num < NUM_STAT) ret = 1;
    return ret;
}
#else
static unsigned char stactrl_is_need(struct rtl8192cd_priv *priv, unsigned char *mac)
{
    struct stactrl_preferband_entry * prefer_ent;
    unsigned char ret = 0;
    unsigned long   flags=0;

    SAVE_INT_AND_CLI(flags);
    SMP_LOCK_STACONTROL_LIST(flags);

    prefer_ent = stactrl_preferband_sta_lookup(priv, mac);
    if(prefer_ent)
    {
        assoc_num = GET_ROOT(priv)->assoc_num;
        #ifdef MBSSID
        if (GET_ROOT(priv)->pmib->miscEntry.vap_enable){
            for (i=0; i<RTL8192CD_NUM_VWLAN; i++) {
                priv_temp = GET_ROOT(priv)->pvap_priv[i];
                if(priv_temp && IS_DRV_OPEN(priv_temp))
                    assoc_num += priv_temp-> assoc_num;
            }
        }
        #endif	
        #ifdef UNIVERSAL_REPEATER
        priv_temp = GET_VXD_PRIV(GET_ROOT(priv));
        if (priv_temp && IS_DRV_OPEN(priv_temp))
            assoc_num += priv_temp-> assoc_num;
        #endif
        #ifdef WDS
        if(GET_ROOT(priv)->pmib->dot11WdsInfo.wdsEnabled)
            assoc_num ++;
        #endif

        if(assoc_num < NUM_STAT)
        {
			ret = 1;
        }
    }
    SMP_UNLOCK_STACONTROL_LIST(flags);
    RESTORE_INT(flags);
    return ret;
}
#endif // STA_CONTROL_AIPC


#ifdef STA_CONTROL_AIPC
static void stactrl_preferband_expire(struct rtl8192cd_priv *priv)
{
    int           i;
    int           num_sta;
    int           num_sta_ori;
    unsigned long flags = 0;
    struct stactrl_prefer_shm prefer;

    SAVE_INT_AND_CLI(flags);
    SMP_LOCK_STACONTROL_LIST(flags);

    aipc_wlan_get(STACTRL_WLAN_ID, (char *)&prefer.prefer_num, STACTRL_SHM_PREFER_NUM, 1, 0);
    aipc_wlan_get(STACTRL_WLAN_ID, (char *)&prefer.entry, STACTRL_SHM_PREFER_LIST, STACTRL_PREFERLIST_SIZE, 0);
    num_sta_ori = num_sta = prefer.prefer_num;
    /* for each station, plus aging by 1 */
    for (i = 0; i < num_sta; i++) {
        prefer.entry[i].aging++;
        /* check if the station expires */
        if (prefer.entry[i].aging > STACTRL_PREFERBAND_EXPIRE) {
            prefer.entry[i].used = 0;
        }
    }
    /* replace the expiring station with the last one in the prefer-band list */
    for (i = 0; i < num_sta; i++) {
        if (0 == prefer.entry[i].used) {
            prefer.entry[i] = prefer.entry[num_sta - 1];
            memset((void *)&prefer.entry[num_sta - 1], 0, STACTRL_PREFER_ENTRY_SIZE);
            i -= 1;
            num_sta -= 1;
            prefer.prefer_num--;
        }
    }
    aipc_wlan_set(STACTRL_WLAN_ID, (char *)&prefer.prefer_num, STACTRL_SHM_PREFER_NUM, 1, 0);
    aipc_wlan_set(STACTRL_WLAN_ID, (char *)&prefer.entry, STACTRL_SHM_PREFER_LIST, num_sta_ori * STACTRL_PREFER_ENTRY_SIZE, 0);

    SMP_UNLOCK_STACONTROL_LIST(flags);
    RESTORE_INT(flags);
}
#else
static void stactrl_preferband_expire(struct rtl8192cd_priv *priv)
{
    int i;
    unsigned long offset;
    struct stactrl_link_list *link, *temp_link;
    struct stactrl_preferband_entry * ent;
    unsigned long flags = 0;
    SAVE_INT_AND_CLI(flags);
    SMP_LOCK_STACONTROL_LIST(flags);

    offset = (unsigned long)(&((struct stactrl_preferband_entry *)0)->link_list);

    for (i=0; i<STACTRL_PREFERBAND_HASH_SIZE; i++)
    {
        link = priv->stactrl.stactrl_preferband_machash[i];
        while (link != NULL)
        {
            temp_link = link->next_hash;
            ent = (struct stactrl_preferband_entry *)((unsigned long)link - offset);
            if(ent->used)
            {
                ent->aging++;
                if(ent->aging > STACTRL_PREFERBAND_EXPIRE)
                {
                    ent->used = 0;
                    stactrl_mac_hash_unlink(link);
                }
            }
            link = temp_link;
        }
    }
    SMP_UNLOCK_STACONTROL_LIST(flags);
    RESTORE_INT(flags);

}
#endif // CONFIG_LUNA_DULA_LINUX


#ifdef STA_CONTROL_AIPC
void stactrl_non_prefer_expire(unsigned long task_priv)
{
    struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)task_priv;
    int                    i;
    int                    num_sta;
    int                    num_sta_ori;
    unsigned long          flags = 0;
    struct stactrl_block_shm block;

    SAVE_INT_AND_CLI(flags);
    SMP_LOCK_STACONTROL_LIST(flags);

    aipc_wlan_get(STACTRL_WLAN_ID, (char *)&block.block_num, STACTRL_SHM_BLOCK_NUM, 1, 0);
    aipc_wlan_get(STACTRL_WLAN_ID, (char *)&block.entry, STACTRL_SHM_BLOCK_LIST, STACTRL_BLOCKLIST_SIZE, 0);
    num_sta_ori = num_sta = block.block_num;

    for (i = 0; i < num_sta; i++) {
        if (1 == block.entry[i].used) {
            if (block.entry[i].timerZ) {
                block.entry[i].timerZ--;
                if (0 == block.entry[i].timerZ) {
                    block.entry[i].used = 0;
                }
            } else if (block.entry[i].timerX) {
                block.entry[i].timerX--;
                if (0 == block.entry[i].timerX) {
                    block.entry[i].timerZ = STACTRL_BLOCK_EXPIRE_Z;
                }
            }
        }
    }

    for (i = 0; i < num_sta; i++) {
        if (0 == block.entry[i].used) {
            block.entry[i] = block.entry[num_sta - 1];
            memset((void *)&block.entry[num_sta - 1], 0, STACTRL_BLOCK_ENTRY_SIZE);
            i -= 1;
            num_sta -= 1;
            block.block_num -= 1;
        }
    }
    aipc_wlan_set(STACTRL_WLAN_ID, (char *)&block.block_num, STACTRL_SHM_BLOCK_NUM, 1, 0);
    aipc_wlan_set(STACTRL_WLAN_ID, (char *)&block.entry, STACTRL_SHM_BLOCK_LIST, num_sta_ori * STACTRL_BLOCK_ENTRY_SIZE, 0);
	
    RESTORE_INT(flags);
    SMP_UNLOCK_STACONTROL_LIST(flags);

#if STA_CONTROL_ALGO == STA_CONTROL_ALGO2
    mod_timer(&priv->stactrl.stactrl_timer, jiffies + RTL_MILISECONDS_TO_JIFFIES(STACTRL_NON_PREFER_TIMER));
#endif
}
#else
void stactrl_non_prefer_expire(unsigned long task_priv)
{
    struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)task_priv;
    int i;
    unsigned long offset;
    struct stactrl_link_list *link, *temp_link;
    struct stactrl_block_entry * ent;
    unsigned long flags = 0;

    offset = (unsigned long)(&((struct stactrl_block_entry *)0)->link_list);

    SAVE_INT_AND_CLI(flags);
    SMP_LOCK_STACONTROL_LIST(flags);

    for (i=0; i<STACTRL_BLOCK_HASH_SIZE; i++)
    {
        link = priv->stactrl.stactrl_block_machash[i];
        while (link != NULL)
        {
            temp_link = link->next_hash;
            ent = (struct stactrl_block_entry *)((unsigned long)link - offset);
            if(ent->timerZ)
            {
                ent->timerZ--;
                if(ent->timerZ == 0)
                {
                    ent->used = 0;
                    stactrl_mac_hash_unlink(link);
                    STACTRL_DEBUG(ent->mac, "block delete");
                }
            }
            else if(ent->timerX)
            {
                ent->timerX--;

                if(ent->timerX == 0)
                {
                    ent->timerZ = STACTRL_BLOCK_EXPIRE_Z;
                    STACTRL_DEBUG(ent->mac, "block Z stage");
                }
            }

            link = temp_link;
        }
    }

    RESTORE_INT(flags);
    SMP_UNLOCK_STACONTROL_LIST(flags);

#if STA_CONTROL_ALGO == STA_CONTROL_ALGO2
    mod_timer(&priv->stactrl.stactrl_timer, jiffies + RTL_MILISECONDS_TO_JIFFIES(STACTRL_NON_PREFER_TIMER));
#endif
}
#endif // STA_CONTROL_AIPC


void stactrl_expire(struct rtl8192cd_priv *priv)
{
    if(priv->stactrl.stactrl_prefer)
        stactrl_preferband_expire(priv);
#if STA_CONTROL_ALGO == STA_CONTROL_ALGO1
    if(0 == priv->stactrl.stactrl_prefer)
        stactrl_non_prefer_expire((unsigned long) priv);
#endif
}

#ifdef STA_CONTROL_AIPC
#if STA_CONTROL_ALGO == STA_CONTROL_ALGO1
/*return: 1: ignore, 0: continu process, 2: error code*/
unsigned char stactrl_check_request(struct rtl8192cd_priv *priv, unsigned char *mac, int frame_type)
{
    int            index = NOT_EXIST;
    int            ret = 0;
    unsigned long  flags = 0;
    struct stactrl_block_shm block;

    SAVE_INT_AND_CLI(flags);
    SMP_LOCK_STACONTROL_LIST(flags);

    index = stactrl_block_lookup(priv, mac, &block);
    if (block.status) {
        /* if find the entry in the block-band list */
        if (NOT_EXIST != index) {
            /* check the timerX and retryY */
            if (block.entry[index].timerX && block.entry[index].retryY) {
                if (frame_type == WIFI_PROBEREQ) {
                    ret = 1;
                } else if (frame_type == WIFI_ASSOCREQ) {
                    ret = 2;
                    block.entry[index].retryY--;
                    if (block.entry[index].retryY == 0) {
                        block.entry[index].timerZ = STACTRL_BLOCK_EXPIRE_Z;
                    }
                    aipc_wlan_set(STACTRL_WLAN_ID, (char *)&block.entry[index], 
                        STACTRL_SHM_BLOCK_LIST + index * STACTRL_BLOCK_ENTRY_SIZE, STACTRL_BLOCK_ENTRY_SIZE, 0);
                }
            }
        } else {
            /* if didn't find any entry in the block-band list */
            if (stactrl_is_need(priv, mac)) {
                ret = 1;
                if (stactrl_block_add(priv, mac, &block)) {
                    STACTRL_DEBUG(mac, "block add");
                } else {
                    STACTRL_DEBUG(mac, "block add fail");
                }
            }
        }
    } else {
        priv->stactrl.stactrl_status = 0;
    }
	
    SMP_UNLOCK_STACONTROL_LIST(flags);
    RESTORE_INT(flags);

    STACTRL_DEBUG(mac, "receive %s drop: %d", (frame_type == WIFI_PROBEREQ? "probe": (frame_type == WIFI_AUTH? "auth": "assoc")),ret);
    return ret;
}
#elif STA_CONTROL_ALGO == STA_CONTROL_ALGO2
/*return: 1: ignore, 0: continu process,  2: error code*/
unsigned char stactrl_check_request(struct rtl8192cd_priv *priv, unsigned char *mac, int frame_type)
{
    int index = NOT_EXIST, ret;
    struct stactrl_block_shm block;

    SAVE_INT_AND_CLI(flags);
    SMP_LOCK_STACONTROL_LIST(flags);

    index = stactrl_block_lookup(priv, mac, &block);

    if (block.status) {
        if (NOT_EXIST != index) {
            /* check the timerX and retryY */
            if (block.entry[index].timerZ) {
                if (frame_type == WIFI_ASSOCREQ) {
                    if (block.entry[index].retryY) {
                        block.entry[index].retryY--;
                        aipc_wlan_set(STACTRL_WLAN_ID, (char *)&block.entry[index], 
                            STACTRL_SHM_BLOCK_LIST + index * STACTRL_BLOCK_ENTRY_SIZE, STACTRL_BLOCK_ENTRY_SIZE, 0);
                        ret = 2;
                    }
                }
            } else {
                ret = 1;
            }
        } else {
            /* if didn't find any entry in the block-band list */
            if (stactrl_is_need(priv, mac)) {
                ret = 1;
                if (frame_type == WIFI_AUTH || frame_type == WIFI_ASSOCREQ) {
                    if (stactrl_block_add(priv, mac, &block)) {
                        STACTRL_DEBUG(mac, "block add");
                    } else {
                        STACTRL_DEBUG(mac, "block add fail");
                    }
                }
            }
        }
    } else {
        priv->stactrl.stactrl_status = 0;
    }

    
    SMP_UNLOCK_STACONTROL_LIST(flags);
    RESTORE_INT(flags);

    STACTRL_DEBUG(mac, "receive %s drop: %d", (frame_type == WIFI_PROBEREQ? "probe": (frame_type == WIFI_AUTH? "auth": "assoc")),ret);

    return ret;
}
#endif // STA_CONTROL_ALGO
#else
#if STA_CONTROL_ALGO == STA_CONTROL_ALGO1
/*return: 1: ignore, 0: continu process, 2: error code*/
unsigned char stactrl_check_request(struct rtl8192cd_priv *priv, unsigned char *mac, int frame_type)
{
    struct stactrl_block_entry * block_ent;
    unsigned char ret = 0;
    unsigned long flags = 0;
    SAVE_INT_AND_CLI(flags);
    SMP_LOCK_STACONTROL_LIST(flags);
    block_ent =  stactrl_block_lookup(priv, mac);
    if(block_ent)
    {
        if(block_ent->timerX && block_ent->retryY)
        {
            if(frame_type == WIFI_PROBEREQ)
            {
                ret = 1;
            }
            else if(frame_type == WIFI_ASSOCREQ)
            {
                ret = 2;
                block_ent->retryY--;

                if(block_ent->retryY == 0)
                {
                    block_ent->timerZ = STACTRL_BLOCK_EXPIRE_Z;
                    STACTRL_DEBUG(block_ent->mac, "block Z stage");
                }
            }
        }
        SMP_UNLOCK_STACONTROL_LIST(flags);
    }
    else
    {
        SMP_UNLOCK_STACONTROL_LIST(flags);
        if(stactrl_is_need(priv->stactrl.stactrl_priv_sc, mac))
        {
            ret = 1;
            SMP_LOCK_STACONTROL_LIST(flags);
            block_ent = stactrl_block_add(priv, mac);
            if(block_ent)   /*add success*/
            {
                STACTRL_DEBUG(mac, "block add");
            }
            else
            {
                STACTRL_DEBUG(mac, "block add fail");
            }
            SMP_UNLOCK_STACONTROL_LIST(flags);
        }
    }
    RESTORE_INT(flags);

    STACTRL_DEBUG(mac, "receive %s drop: %d", (frame_type == WIFI_PROBEREQ? "probe": (frame_type == WIFI_AUTH? "auth": "assoc")),ret);

    return ret;
}


#elif STA_CONTROL_ALGO == STA_CONTROL_ALGO2
/*return: 1: ignore, 0: continu process,  2: error code*/
unsigned char stactrl_check_request(struct rtl8192cd_priv *priv, unsigned char *mac, int frame_type)
{
    struct stactrl_block_entry * block_ent;
    unsigned char ret = 0;

    unsigned long flags = 0;
    SAVE_INT_AND_CLI(flags);
    SMP_LOCK_STACONTROL_LIST(flags);

    block_ent =  stactrl_block_lookup(priv, mac);
    if(block_ent)
    {
        if(block_ent->timerZ)
        {
            if(frame_type == WIFI_ASSOCREQ)
            {
                if(block_ent->retryY)
                {
                    block_ent->retryY--;
                    ret = 2;
                }
            }
        }
        else
        {
            ret = 1;
        }
    }
    else
    {
        if(stactrl_is_need(priv->stactrl.stactrl_priv_sc, mac))
        {
            ret = 1;
            if(frame_type == WIFI_AUTH || frame_type == WIFI_ASSOCREQ)
            {
                block_ent = stactrl_block_add(priv, mac);
                if(block_ent)   /*add success*/
                {
                    STACTRL_DEBUG(mac, "block add");
                }
                else
                {
                    STACTRL_DEBUG(mac, "block add fail");
                }
            }
        }
    }
    SMP_UNLOCK_STACONTROL_LIST(flags);
    RESTORE_INT(flags);

    STACTRL_DEBUG(mac, "receive %s drop: %d", (frame_type == WIFI_PROBEREQ? "probe": (frame_type == WIFI_AUTH? "auth": "assoc")),ret);

    return ret;
}
#endif // STA_CONTROL_ALGO
#endif // STA_CONTROL_AIPC
#endif /* STA_CONTROL */
