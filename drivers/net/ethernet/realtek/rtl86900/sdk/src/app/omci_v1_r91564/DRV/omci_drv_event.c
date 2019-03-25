/*
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 *
 *
 * $Revision: 42199 $
 * $Date: 2013-08-20 12:00:44 +0800 (Tue, 20 Aug 2013) $
 *
 * Purpose : OMCI Drive for Event handler
 *
 * Feature : Provide the Event Handler for control VEIP
 *
 */
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/rtnetlink.h>
#include <linux/notifier.h>
#include <linux/list.h>
#include <net/rtnetlink.h>
#include <net/netns/generic.h>
#include <linux/etherdevice.h>
#include <net/netfilter/nf_conntrack_ecache.h>
#include <net/netevent.h>
#include <linux/inetdevice.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/ipv4/nf_conntrack_ipv4.h>
#include <net/neighbour.h>
#include <net/route.h>
#include <net/sock.h>
#include <net/ip_fib.h>
#include <net/if_inet6.h>
#include <net/addrconf.h>
#include <OMCI/inc/omci_driver.h>
#include "omci_drv.h"
#ifndef OMCI_X86
#include "rtk/l34.h"
#endif

extern int omcidrv_wrapper_clearPPPoEDb(void);
extern int omcidrv_wrapper_updateVeipRule(int wanIdx, int vid, int pri,
	int type, int service, int isBinding, int netIfIdx, unsigned char isRegister);

#define WAN_INTF_NAME_PREFIX "nas0_"

typedef struct {
	int				vid;		// 0 ~ 4095, -1: disable
	int				pri;		// 0 ~ 7, -1: disable
	int				type;		// -1: disable
	int				service;	// 1: internet
	int				isBinding;	// 1: binding enabled
	int				netIfIdx;	// l34 netIfIdx, for all types of wan intf
	unsigned char	isRuleCfg;	// indicator of if veip rule has configured
} omcidrv_wanIntfMap_t;

static omcidrv_wanIntfMap_t *g_wanIntfMap;
static unsigned int g_wanIntfNum;

static int omcidrv_wrapper_checkWanInfo(int wanIdx)
{
    int				ret;
    unsigned int i;

    if (OMCI_MODE_PPPOE == g_wanIntfMap[wanIdx].type)
    {
    	for (i = 0; i < g_wanIntfNum; i++)
    	{
    	    if (i == wanIdx)
                continue;
    	    if (OMCI_MODE_PPPOE == g_wanIntfMap[i].type)
            {
                return OMCI_ERR_OK;
            }
    	}
        ret = omcidrv_wrapper_clearPPPoEDb();
    }
	return OMCI_ERR_OK;
}


int
omcidrv_updateWanInfoByProcEntry(
    int netIfIdx,
    int vid,
    int pri,
    int type,
    int service,
    int isBinding,
    int bAdd)
{
	int				ret = OMCI_ERR_FAILED;
	unsigned char	isRegister;

	if (netIfIdx < 0 || netIfIdx >= g_wanIntfNum)
    {
		return OMCI_ERR_FAILED;
    }

	if (vid > 0xFFF || pri > 0x8)
    {
		return OMCI_ERR_FAILED;
    }

	isRegister = (bAdd > 0) ? TRUE : FALSE;

    ret = omcidrv_wrapper_updateVeipRule(
        netIfIdx,
        vid,
        pri,
        type,
        service,
        isBinding,
        netIfIdx,
        isRegister);

	if (isRegister)
	{
        OMCI_LOG(
            OMCI_LOG_LEVEL_DBG,
            "%s: modify the wanif[%u]\n",
            __FUNCTION__,
            netIfIdx);
		g_wanIntfMap[netIfIdx].vid = vid;
		g_wanIntfMap[netIfIdx].pri = pri;
		g_wanIntfMap[netIfIdx].type = type;
		g_wanIntfMap[netIfIdx].service = service;
		g_wanIntfMap[netIfIdx].isBinding = isBinding;
		g_wanIntfMap[netIfIdx].netIfIdx = netIfIdx;
        if (ret == OMCI_ERR_OK || ret == OMCI_ERR_ENTRY_EXIST)
        {
            g_wanIntfMap[netIfIdx].isRuleCfg = 1;
        }
        else
        {
            g_wanIntfMap[netIfIdx].isRuleCfg = 0;
        }
	}
	else
	{
        OMCI_LOG(
            OMCI_LOG_LEVEL_DBG,
            "%s: Delete the wanif[%u]\n",
            __FUNCTION__,
            netIfIdx);
	    ret = omcidrv_wrapper_checkWanInfo(netIfIdx);
		g_wanIntfMap[netIfIdx].vid = -1;
		g_wanIntfMap[netIfIdx].pri = -1;
		g_wanIntfMap[netIfIdx].type = -1;
		g_wanIntfMap[netIfIdx].service = -1;
		g_wanIntfMap[netIfIdx].isBinding = FALSE;
		g_wanIntfMap[netIfIdx].netIfIdx = -1;
		g_wanIntfMap[netIfIdx].isRuleCfg = FALSE;
	}

	return OMCI_ERR_OK;
}

int omcidrv_dumpWanInfo(void)
{
	unsigned int i;

	for (i = 0; i < g_wanIntfNum; i++)
	{
		printk("wanif[%u]: vid=%d, pri=%d, type=%d, service=%d, isBinding=%d, netIfIdx=%d, isRuleCfg=%u\n",
			i,
			g_wanIntfMap[i].vid,
			g_wanIntfMap[i].pri,
			g_wanIntfMap[i].type,
			g_wanIntfMap[i].service,
			g_wanIntfMap[i].isBinding,
			g_wanIntfMap[i].netIfIdx,
			g_wanIntfMap[i].isRuleCfg);
	}

	return OMCI_ERR_OK;
}

int omcidrv_alloc_resource(unsigned int intfNum)
{
	unsigned int i;

	if (intfNum == 0)
		return -1;

	g_wanIntfMap = kmalloc(sizeof(omcidrv_wanIntfMap_t) * intfNum, GFP_KERNEL);
	if (!g_wanIntfMap)
		return -1;

	for (i = 0; i < intfNum; i++)
	{
		g_wanIntfMap[i].vid = -1;
		g_wanIntfMap[i].pri = -1;
		g_wanIntfMap[i].type = -1;
		g_wanIntfMap[i].service = -1;
		g_wanIntfMap[i].isBinding = FALSE;
		g_wanIntfMap[i].netIfIdx = -1;
		g_wanIntfMap[i].isRuleCfg = FALSE;
	}

	g_wanIntfNum = intfNum;

	return 0;
}

void omcidrv_dealloc_resource(void)
{
	if (g_wanIntfMap)
		kfree(g_wanIntfMap);

	g_wanIntfNum = 0;
}

int omcidrv_setWanStatusByIfIdx(int				wanIdx,
								unsigned char	isRuleCfg)
{
	if (wanIdx < 0 || wanIdx >= g_wanIntfNum)
		return OMCI_ERR_FAILED;

	g_wanIntfMap[wanIdx].isRuleCfg = isRuleCfg;

	return OMCI_ERR_OK;
}

int omcidrv_getWanInfoByIfIdx(int				wanIdx,
								int				*vid,
								int				*pri,
								int				*type,
								int				*service,
								int				*isBinding,
								int				*netIfIdx,
								unsigned char	*isRuleCfg)
{
	if (wanIdx < 0 || wanIdx >= g_wanIntfNum)
		return OMCI_ERR_FAILED;

	if (!vid || !pri || !type || !service || !isBinding || !netIfIdx || !isRuleCfg)
		return OMCI_ERR_FAILED;

	*vid = g_wanIntfMap[wanIdx].vid;
	*pri = g_wanIntfMap[wanIdx].pri;
	*type = g_wanIntfMap[wanIdx].type;
	*service = g_wanIntfMap[wanIdx].service;
	*isBinding = g_wanIntfMap[wanIdx].isBinding;
	*netIfIdx = g_wanIntfMap[wanIdx].netIfIdx;
	*isRuleCfg = g_wanIntfMap[wanIdx].isRuleCfg;

	return OMCI_ERR_OK;
}

EXPORT_SYMBOL(omcidrv_setWanStatusByIfIdx);
EXPORT_SYMBOL(omcidrv_getWanInfoByIfIdx);

